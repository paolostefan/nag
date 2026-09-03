#ifndef NAG_EDITOR_GRAPH_SCENE_SYNC_H
#define NAG_EDITOR_GRAPH_SCENE_SYNC_H

#include <optional>

#include "editor/scene.h"
#include "engine/node_graph.h"
#include "engine/serialization/json_graph_serializer.h"

/**
 * @brief Syncs graph data between the node graph and the Scene's graph_data store.
 *
 * Handles the pure data movement: serializing a NodeGraph into a Scene's graph_data
 * map keyed by graph id, and deserializing it back out. Also find-or-create graph
 * references in the folder tree. No UI concerns leak in here.
 */
class GraphSceneSync {
public:
  explicit GraphSceneSync(Scene &scene) : scene_(scene) {}

  /// @brief Serialize @p graph into scene.graph_data under ref.id, mark ref clean.
  void sync_graph(GraphReference &ref, const NodeGraph &graph);

  /// @brief Deserialize the graph for @p ref from scene.graph_data.
  /// @return The loaded graph, or std::nullopt when missing/invalid/empty.
  [[nodiscard]] std::optional<NodeGraph> load_graph(const GraphReference &ref) const;

  /// @brief Depth-first search for the first graph reference in the folder tree.
  /// @return Pointer to the first graph, or nullptr if none exist.
  [[nodiscard]] GraphReference *find_first_graph() const;

  /// @brief Create a graph reference in @p folder with empty data, then return it.
  /// @return The new graph reference, or nullptr on failure.
  [[nodiscard]] GraphReference *add_graph(GraphFolder &folder, std::string_view graph_name);

  /// @brief Remove the graph's data for @p graph_id, if present.
  void remove_graph_data(const std::string &graph_id);

private:
  Scene &scene_;
};

#endif // NAG_EDITOR_GRAPH_SCENE_SYNC_H
