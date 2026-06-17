#ifndef NAG_EDITOR_SCENE_LIBRARY_H
#define NAG_EDITOR_SCENE_LIBRARY_H

#include <filesystem>
#include <string>

#include "editor/scene.h"
#include "engine/node_graph.h"
#include "engine/serialization/json_graph_serializer.h"

/**
 *  SceneLibrary manages the saving and loading of scenes and node graphs to and from disk.
 *  It provides an interface for the editor to persist scenes and their associated graphs, as well as to organize
 *  graphs within a directory structure.
 */
class SceneLibrary {
public:
  explicit SceneLibrary(std::filesystem::path scenes_directory);

  [[nodiscard]] const std::filesystem::path &get_scenes_directory() const { return scenes_directory_; }

  [[nodiscard]] bool save_scene(const Scene &scene, const std::filesystem::path &path);

  [[nodiscard]] static std::unique_ptr<Scene> load_scene(const std::filesystem::path &path);

  /// Saves the given graph to disk using the path specified in the reference.
  /// @return true if the save was successful, false otherwise.
  [[nodiscard]] bool save_graph(const NodeGraph &graph, const GraphReference &reference) const;

  /// Loads a graph from disk using the given graph ID.
  /// The method will search for a graph file with the specified ID within the scenes directory and its subdirectories.
  [[nodiscard]] std::unique_ptr<NodeGraph> load_graph(const std::string &graph_id) const;

  /// Loads a graph from disk using the path specified in the reference.
  [[nodiscard]] std::unique_ptr<NodeGraph> load_graph(const GraphReference &reference) const {
    return load_graph(reference.id);
  }

  /// Deletes the graph file associated with the given reference.
  /// @return true if the file was successfully deleted or did not exist, false if an error occurred.
  [[nodiscard]] bool delete_graph(const GraphReference &reference) const;

  [[nodiscard]] static bool rename_graph(const GraphReference &reference, const std::string &new_name);

private:
  std::filesystem::path scenes_directory_;
  JsonGraphSerializer serializer_;
};

#endif // NAG_EDITOR_SCENE_LIBRARY_H
