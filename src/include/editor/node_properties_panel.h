#ifndef NAG_EDITOR_NODE_PROPERTIES_PANEL_H
#define NAG_EDITOR_NODE_PROPERTIES_PANEL_H

#include "editor/command_history.h"

/// @file node_properties_panel.h
/// @brief Collapsible "Node Properties" panel for the graph editor.
///
/// Render() should be called once per frame inside the editor's ImGui window,
/// after all node/link interaction has been processed.

class NodePropertiesPanel {
public:
  /// @brief Render the properties panel.
  ///
  /// Reads the current ImNodes selection, finds the selected node in `graph`,
  /// and delegates rendering to Node::draw_properties().
  ///
  /// @param graph    The active node graph.
  /// @param history  The command history for undo/redo.
  void render(NodeGraph &graph, CommandHistory &history);

private:
  /// Last known selected node ID. Used to detect selection changes.
  int last_selected_id_{-1};
};


#endif //NAG_EDITOR_NODE_PROPERTIES_PANEL_H
