#ifndef NAG_EDITOR_SCENE_LIBRARY_H
#define NAG_EDITOR_SCENE_LIBRARY_H

#include <filesystem>
#include <string>

#include "editor/scene.h"
#include "engine/node_graph.h"
#include "engine/serialization/json_graph_serializer.h"

class SceneLibrary {
public:
  explicit SceneLibrary(std::filesystem::path scenes_directory);

  [[nodiscard]] const std::filesystem::path &get_scenes_directory() const { return scenes_directory_; }

  [[nodiscard]] static bool save_scene(const Scene &scene, const std::filesystem::path &path) ;

  [[nodiscard]] static std::unique_ptr<Scene> load_scene(const std::filesystem::path &path) ;

  [[nodiscard]] bool save_graph(const NodeGraph &graph, const GraphReference &reference) const;

  [[nodiscard]] static std::unique_ptr<NodeGraph> load_graph(const GraphReference &reference) ;

  [[nodiscard]] static std::unique_ptr<NodeGraph> load_graph(const std::string &graph_id);

  [[nodiscard]] bool delete_graph(const GraphReference &reference) const;

  [[nodiscard]] static bool rename_graph(const GraphReference &reference, const std::string &new_name) ;

private:
  std::filesystem::path scenes_directory_;
  JsonGraphSerializer serializer_;
};

#endif // NAG_EDITOR_SCENE_LIBRARY_H
