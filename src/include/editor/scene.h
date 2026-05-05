#ifndef NAG_EDITOR_SCENE_H
#define NAG_EDITOR_SCENE_H

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "imgui.h"
#include "nlohmann/json.hpp"

#include "engine/node_graph.h"

struct GraphReference {
  std::string id;
  std::string name;
  std::filesystem::path path;
  bool dirty{false};

  GraphReference() = default;

  GraphReference(std::string id, std::string name, std::filesystem::path path)
    : id(std::move(id)), name(std::move(name)), path(std::move(path)) {
  }
};

struct GraphFolder {
  std::string id;
  std::string name;
  std::vector<GraphReference> graphs;
  std::vector<std::unique_ptr<GraphFolder> > children;

  GraphFolder() = default;

  GraphFolder(std::string id, std::string name)
    : id(std::move(id)), name(std::move(name)) {
  }
};

struct TimelineSegment {
  int frame_start{0};
  int frame_end{100};
  std::string graph_id;
  ImU32 color{0xFF8080FF};

  TimelineSegment() = default;

  TimelineSegment(const int start, const int end, std::string graph_id, const ImU32 color = 0xFF8080FF)
    : frame_start(start), frame_end(end), graph_id(std::move(graph_id)), color(color) {
  }
};

class Scene {
public:
  // Format version for compatibility checks during loading
  static constexpr auto kFormatVersion = "1.0";

  std::string name{"Untitled Scene"};
  std::filesystem::path path{};
  std::vector<std::unique_ptr<GraphFolder> > root_folders;
  std::vector<TimelineSegment> timeline;
  int fps{60};
  int total_frames{1000};
  bool pristine{true};

  Scene() = default;

  explicit Scene(std::string name) : name(std::move(name)) {
  }

  [[nodiscard]] static std::string generate_id();

  [[nodiscard]] GraphFolder *find_folder(const std::string &folder_id);

  [[nodiscard]] const GraphFolder *find_folder(const std::string &folder_id) const;

  [[nodiscard]] GraphReference *find_graph(const std::string &graph_id);

  [[nodiscard]] const GraphReference *find_graph(const std::string &graph_id) const;

  // TODO: stop using std::optional, use C-style empty string
  [[nodiscard]] GraphFolder *add_folder(const std::optional<std::string> &parent_id,
                                        const std::string &folder_name);

  /// @brief Rename a folder in the library. Does not affect the graph file or timeline segments.
  /// @return true if the folder was found and renamed, false otherwise.
  bool rename_folder(const std::string &folder_id, const std::string &new_name);


  bool remove_folder(const std::string &folder_id);

  [[nodiscard]] GraphReference *add_graph(const std::string &folder_id,
                                          const std::string &graph_name,
                                          const NodeGraph &graph);

  bool remove_graph(const std::string &graph_id);

  bool move_graph(const std::string &graph_id, const std::string &target_folder_id);

  /// @brief Rename a graph in the library. Does not affect the graph file or timeline segments.
  /// @return true if the graph was found and renamed, false otherwise.
  bool rename_graph(const std::string &graph_id, const std::string &new_name);

  void add_timeline_segment(const TimelineSegment &segment);

  void remove_timeline_segment(size_t index);

  void update_timeline_segment(size_t index, const TimelineSegment &segment);

  [[nodiscard]] int get_frame_at(int frame) const;

  [[nodiscard]] const GraphReference *get_graph_at_frame(int frame) const;
};

inline void to_json(nlohmann::json &j, const GraphReference &ref) {
  j = nlohmann::json{
    {"id", ref.id},
    {"name", ref.name},
    {"path", ref.path.string()},
    {"dirty", ref.dirty}
  };
}

inline void from_json(const nlohmann::json &j, GraphReference &ref) {
  j.at("id").get_to(ref.id);
  j.at("name").get_to(ref.name);
  ref.path = j.at("path").get<std::filesystem::path>();
  if (j.contains("dirty"))
    j.at("dirty").get_to(ref.dirty);
}

inline void to_json(nlohmann::json &j, const GraphFolder &folder) {
  j = nlohmann::json{{"id", folder.id}, {"name", folder.name}, {"graphs", folder.graphs}};
  for (const auto &child: folder.children) {
    j["children"].push_back(*child);
  }
}

inline void from_json(const nlohmann::json &j, GraphFolder &folder) {
  j.at("id").get_to(folder.id);
  j.at("name").get_to(folder.name);
  j.at("graphs").get_to(folder.graphs);
  if (j.contains("children")) {
    for (const auto &child_json: j.at("children")) {
      auto child = std::make_unique<GraphFolder>();
      child_json.get_to(*child);
      folder.children.push_back(std::move(child));
    }
  }
}

inline void to_json(nlohmann::json &j, const TimelineSegment &segment) {
  j = nlohmann::json{
    {"frame_start", segment.frame_start},
    {"frame_end", segment.frame_end},
    {"graph_id", segment.graph_id},
    {"color", segment.color}
  };
}

inline void from_json(const nlohmann::json &j, TimelineSegment &segment) {
  j.at("frame_start").get_to(segment.frame_start);
  j.at("frame_end").get_to(segment.frame_end);
  j.at("graph_id").get_to(segment.graph_id);
  if (j.contains("color"))
    j.at("color").get_to(segment.color);
}

inline void to_json(nlohmann::json &j, const Scene &scene) {
  j = nlohmann::json{
    {"version", Scene::kFormatVersion},
    {"name", scene.name},
    {"fps", scene.fps},
    {"total_frames", scene.total_frames},
    {"timeline", scene.timeline}
  };
  for (const auto &folder: scene.root_folders) {
    j["folders"].push_back(*folder);
  }
}

inline void from_json(const nlohmann::json &j, Scene &scene) {
  if (j.contains("version"))
    j.at("version").get_to(scene.name);
  if (j.contains("name"))
    j.at("name").get_to(scene.name);
  if (j.contains("fps"))
    j.at("fps").get_to(scene.fps);
  if (j.contains("total_frames"))
    j.at("total_frames").get_to(scene.total_frames);
  if (j.contains("timeline"))
    j.at("timeline").get_to(scene.timeline);

  scene.root_folders.clear();
  if (j.contains("folders")) {
    for (const auto &folder_json: j.at("folders")) {
      auto folder = std::make_unique<GraphFolder>();
      folder_json.get_to(*folder);
      scene.root_folders.push_back(std::move(folder));
    }
  }
}

#endif // NAG_EDITOR_SCENE_H
