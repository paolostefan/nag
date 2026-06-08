#ifndef NAG_EDITOR_GRAPH_EDITOR_H
#define NAG_EDITOR_GRAPH_EDITOR_H

#include <unordered_set>

#include "engine/node_graph.h"
#include "editor/command_history.h"
#include "engine/nodes/generator_nodes.h"
#include "engine/nodes/output_node.h"
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

  /** If true, executed commands will be pushed to the command history for undo/redo support. */
  std::atomic<bool> history_enabled{false};

  /// Command history for undo/redo support. Graph actions should push commands to this history when appropriate.
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

  virtual void build_default_graph();

  /**
   * @brief Loads a graph from the given path, replacing the current one.
   *
   * @param path Absolute or relative path to the JSON file
   */
  OperationResult load_graph(const std::string &path);

  /**
   * @brief Spawns a new node of the given type at a canvas position.
   *
   * The node is created via the registry and added to the graph, and an AddNodeCommand
   * is pushed to the command history for undo/redo support, unless the @ref history_enabled flag is false.
   *
   * @param type     Node type to create via registry
   * @param position Screen-space position for the new node
   *
   * @return The newly created node, or nullptr if creation failed (e.g. unknown type).
   */
  Node *spawn_node(NodeType type, const ImVec2 &position);

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
