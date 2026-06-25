#ifndef NAG_EDITOR_SCENE_H
#define NAG_EDITOR_SCENE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include "nlohmann/json.hpp"

#include "engine/audio_track.h"
#include "engine/node_graph.h"

enum class SegmentType : uint8_t { GRAPH, AUDIO };

struct GraphReference {
  std::string id;
  std::string name;
  bool dirty{false};

  GraphReference() = default;

  GraphReference(std::string id_, std::string name_)
    : id(std::move(id_)), name(std::move(name_)) {
  }
};

struct GraphFolder {
  std::string id;
  std::string name;
  std::vector<GraphReference> graphs;
  std::vector<std::unique_ptr<GraphFolder> > children;

  GraphFolder() = default;

  GraphFolder(const std::string_view id_, const std::string_view name_)
    : id(id_), name(name_) {
  }
};

struct TimelineSegment {
  SegmentType type{SegmentType::GRAPH};
  std::string graph_id;
  int audio_track_index{-1};
  int frame_start{0};
  int frame_end{100};
  ImU32 color{0xFF8080FF};

  TimelineSegment() = default;

  TimelineSegment(const int start, const int end, const std::string_view graph_id, const ImU32 color = 0xFF8080FF)
    : type(SegmentType::GRAPH), graph_id(graph_id), frame_start(start), frame_end(end), color(color) {
  }

  TimelineSegment(const int start, const int end, const int audio_idx, const ImU32 color = 0xFF80FF80)
    : type(SegmentType::AUDIO), audio_track_index(audio_idx), frame_start(start), frame_end(end), color(color) {
  }
};

class Scene {
public:
  // Format version for compatibility checks during loading
  static constexpr auto kFormatVersion = "0.9";

  std::string name{"Untitled Scene"};
  std::filesystem::path path{};
  std::vector<std::unique_ptr<GraphFolder> > root_folders;
  std::vector<TimelineSegment> timeline;
  std::vector<AudioTrack> audio_tracks;
  std::unordered_map<std::string, nlohmann::json> graph_data;
  int fps{60};
  int total_frames{1000};
  bool dirty{false};

  Scene() = default;

  explicit Scene(std::string name) : name(std::move(name)) {
  }

  [[nodiscard]] static std::string generate_id();

  [[nodiscard]] GraphFolder *find_folder(const std::string &folder_id);

  [[nodiscard]] const GraphFolder *find_folder(const std::string &folder_id) const;

  [[nodiscard]] GraphReference *find_graph(const std::string &graph_id);

  [[nodiscard]] const GraphReference *find_graph(const std::string &graph_id) const;

  [[nodiscard]] GraphFolder *add_folder(const char *parent_folder_id,
                                        const std::string &folder_name);

  [[nodiscard]] GraphFolder *add_folder(const std::string &parent_folder_id,
                                        const std::string &folder_name) {
    return add_folder(parent_folder_id.empty() ? nullptr : parent_folder_id.c_str(), folder_name);
  }

  /// @brief Rename a folder in the library. Does not affect the graph file or timeline segments.
  /// @return true if the folder was found and renamed, false otherwise.
  bool rename_folder(const std::string &folder_id, const std::string &new_name);


  bool remove_folder(const std::string &folder_id);

  /**
   *  Adds a graph reference to the specified folder in the library.
   *  The graph is not saved to disk by this method; it only creates a reference in
   *  the scene's folder structure. The caller is responsible for saving the
   *  graph to disk using the SceneLibrary after adding it to the scene.
   *
   * @param folder
   * @param graph_name
   * @return
   */
  [[nodiscard]] GraphReference *add_graph(GraphFolder &folder,
                                          const std::string &graph_name);

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

  size_t add_audio_track(AudioTrack &&track);

  bool remove_audio_track(size_t index);

  [[nodiscard]] AudioTrack *get_audio_track(size_t index);

  [[nodiscard]] const AudioTrack *get_audio_track(size_t index) const;
};

inline void to_json(nlohmann::json &j, const GraphReference &ref) {
  j = nlohmann::json{
    {"id", ref.id},
    {"name", ref.name},
    {"dirty", ref.dirty}
  };
}

inline void from_json(const nlohmann::json &j, GraphReference &ref) {
  j.at("id").get_to(ref.id);
  j.at("name").get_to(ref.name);
  if (j.contains("dirty"))
    j.at("dirty").get_to(ref.dirty);
}

// Note: to_json/from_json for GraphFolder are recursive to handle nested folders
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

NLOHMANN_JSON_SERIALIZE_ENUM(SegmentType, {
  {SegmentType::GRAPH, "Graph"},
  {SegmentType::AUDIO, "Audio"},
});

inline void to_json(nlohmann::json &j, const TimelineSegment &segment) {
  j = nlohmann::json{
    {"type", segment.type},
    {"frame_start", segment.frame_start},
    {"frame_end", segment.frame_end},
    {"graph_id", segment.graph_id},
    {"audio_track_index", segment.audio_track_index},
    {"color", segment.color}
  };
}

inline void from_json(const nlohmann::json &j, TimelineSegment &segment) {
  if (j.contains("type"))
    j.at("type").get_to(segment.type);
  j.at("frame_start").get_to(segment.frame_start);
  j.at("frame_end").get_to(segment.frame_end);
  j.at("graph_id").get_to(segment.graph_id);
  if (j.contains("audio_track_index"))
    j.at("audio_track_index").get_to(segment.audio_track_index);
  if (j.contains("color"))
    j.at("color").get_to(segment.color);
}

/// @brief Custom to_json/from_json for Scene to handle the root_folders vector of unique_ptrs
inline void to_json(nlohmann::json &j, const Scene &scene) {
  j = nlohmann::json{
    {"version", Scene::kFormatVersion},
    {"name", scene.name},
    {"fps", scene.fps},
    {"total_frames", scene.total_frames},
    {"timeline", scene.timeline},
    {"audio_tracks", scene.audio_tracks}
  };
  for (const auto &folder: scene.root_folders) {
    j["folders"].push_back(*folder);
  }
  if (!scene.graph_data.empty()) {
    j["graph_data"] = scene.graph_data;
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

  if (j.contains("audio_tracks"))
    j.at("audio_tracks").get_to(scene.audio_tracks);

  scene.root_folders.clear();
  if (j.contains("folders")) {
    for (const auto &folder_json: j.at("folders")) {
      auto folder = std::make_unique<GraphFolder>();
      folder_json.get_to(*folder);
      scene.root_folders.push_back(std::move(folder));
    }
  }

  scene.graph_data.clear();
  if (j.contains("graph_data")) {
    j.at("graph_data").get_to(scene.graph_data);
  }
}

#endif // NAG_EDITOR_SCENE_H
