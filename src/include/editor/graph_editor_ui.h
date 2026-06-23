#ifndef NAG_EDITOR_GRAPH_EDITOR_UI_H
#define NAG_EDITOR_GRAPH_EDITOR_UI_H

#include "imnodes.h"
#include "editor/node_properties_panel.h"
#include "editor/ui_window.h"
#include "editor/graph_editor.h"
#include "editor/preview_window.h"
#include "engine/nodes/visual_node.h"


/**
 * @class GraphEditorUI
 * @brief A complete node graph editor UI component combining UIWindow and GraphEditor.
 *
 * This class provides a full-featured node editor with:
 * - Visual node graph editing via ImNodes
 * - File save/load functionality
 * - Node creation/deletion with undo/redo support
 * - Preview window for visual nodes
 * - Time-based evaluation system
 */
class GraphEditorUI : public UIWindow, public GraphEditor {
public:
  explicit GraphEditorUI(std::string title = "Graph Editor", int width = 1024, int height = 768);

  ~GraphEditorUI() override = default;

protected:
  void main_event_loop() override;

  void render_ui() override;

  // ── Rendering ─────────────────────────────────────────────────────────────

  void render_node_props() {
    if (ImGui::Begin("Node properties")) {
      node_properties_panel_.render(graph, command_history);
    }
    ImGui::End(); // Node properties
  }

  /**
   * @brief Renders context menus (add node, delete selection).
   */
  virtual void render_context_menu();

  /**
   * @brief Renders the main node editor area using ImNodes.
   */
  void render_node_editor();

  void display_dialogs() override;

  /**
 * @brief Deletes selected nodes and their associated links.
 *
 * Safely removes nodes from the graph and cleans up any links
 * that reference the deleted nodes.
 */
  void delete_selected_nodes();

  /**
   *  @brief Clones the selected nodes
   */
  void duplicate_selected_nodes();

  /**
   * @brief Deletes selected links.
   */
  void delete_selected_links();

  /**
   * @brief Preview window for displaying visual node outputs.
   */
  PreviewWindow preview_window;

  std::atomic<bool> is_time_flowing{true};

private:
  /**
   * @brief Renders the menu bar with File, Edit, and View menus.
   */
  virtual void render_menu_bar();

  /**
   * @brief Renders a visual preview of a visual node in the editor.
   *
   * @param visual_node The visual node to render (must be non-null)
   */
  static void render_visual_node_body(const VisualNode *visual_node);

  /**
   * @brief Gets the color for a pin based on its data type.
   *
   * @param pin The pin to get the color for
   * @return ImColor for the pin
   */
  static unsigned int get_pin_color(const Pin &pin);

  // ── Graph Actions ─────────────────────────────────────────────────────────

  /**
   * @brief Saves the current graph to the given path.
   *
   * @param path Absolute or relative path to the output JSON file
   */
  void save_graph(const std::string &path);



  // ── ImNodes State ─────────────────────────────────────────────────────────
  ImNodesEditorContext *editor_context{nullptr};

  // ── UI State ──────────────────────────────────────────────────────────────

  /**
   * @brief Node properties panel for inspecting/editing selected nodes.
   */
  NodePropertiesPanel node_properties_panel_;

  static constexpr auto kSelNodePopup{"selected_node_popup"};
  static constexpr auto kSaveGraphDialogKey{"SaveGraphDlg"};
  static constexpr auto kLoadGraphDialogKey{"LoadGraphDlg"};
  static constexpr auto kFileFilter{"JSON files{.json},All files{.*}"};
};


#endif // NAG_EDITOR_GRAPH_EDITOR_UI_H
