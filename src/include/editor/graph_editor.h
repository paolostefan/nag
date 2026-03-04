#ifndef NAG_EDITOR_GRAPH_EDITOR_H
#define NAG_EDITOR_GRAPH_EDITOR_H

#include <unordered_set>

#include "engine/node_graph.h"
#include "editor/command_history.h"
#include "engine/nodes/generator_nodes.h"
#include "engine/nodes/visual_nodes.h"
#include "engine/serialization/json_graph_serializer.h"


class GraphEditor {
public:
  virtual ~GraphEditor() = default;

  GraphEditor();

protected:
  NodeGraph graph;

  TimeNode *time_node{};
  OutputNode *output_node{};

  /** True after a graph change (load/reset/undo). */
  std::atomic<bool> node_pos_refresh{true};

  CommandHistory command_history;

  // ── Serialization ─────────────────────────────────────────────────────────
  JsonGraphSerializer serializer;

  // ── Graph Actions ─────────────────────────────────────────────────────────

  /**
   * @brief Clears the graph and resets all dependent state.
   *
   * Invalidates stream pointers and resets the ImNodes first-render flag.
   */
  virtual void reset_graph();

  void build_default_graph();

  /**
   * @brief Loads a graph from the given path, replacing the current one.
   *
   * @param path Absolute or relative path to the JSON file
   */
  OperationResult load_graph(const std::string &path);

  /**
   * @brief Spawns a new node of the given type at a canvas position.
   *
   * @param type     Node type to create via registry
   * @param position Screen-space position for the new node
   */
  void spawn_node(NodeType type, const ImVec2 &position);

  /**
   * @brief Deletes specified nodes and their associated links.
   *
   * Safely removes nodes from the graph and cleans up any links
   * that reference the deleted nodes. Invalidates stream pointers
   * if any deleted node was being plotted.
   */
  void delete_nodes(const std::unordered_set<int> &node_ids_to_delete);

  /**
   * @brief Deletes selected links.
   */
  void delete_links(const std::unordered_set<int> &link_ids_to_delete);
};

#endif // NAG_EDITOR_GRAPH_EDITOR_H
