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

  /**
   * @brief Renders the menu bar with File, Edit, and View menus.
   */
  virtual void render_menu_bar();

  /**
   * @brief Renders context menus (add node, delete selection).
   */
  virtual void render_context_menu();

  /**
   * @brief Renders the main node editor area using ImNodes.
   */
  void render_node_editor(bool with_menu);

private:
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

  /**
   * @brief Deletes selected nodes and their associated links.
   *
   * Safely removes nodes from the graph and cleans up any links
   * that reference the deleted nodes.
   */
  void delete_selected_nodes();

  /**
   * @brief Deletes selected links.
   */
  void delete_selected_links();


  // ── Graph State ───────────────────────────────────────────────────────────

  /**
   * @brief Preview window for displaying visual node outputs.
   */
  PreviewWindow preview_window;

  // ── ImNodes State ─────────────────────────────────────────────────────────
  ImNodesEditorContext *editor_context{nullptr};

  // ── UI State ──────────────────────────────────────────────────────────────

  /**
   * @brief Node properties panel for inspecting/editing selected nodes.
   */
  NodePropertiesPanel node_properties_panel_;

  /**
   * @brief Feedback message shown after save/load operations.
   *
   * Empty string means no message to show.
   */
  std::string status_message;

  /** Timestamp (ImGui time) when status_message was set. */
  float status_message_time{0.f};

  static constexpr float kStatusMessageDuration{3.f}; // seconds
  static constexpr auto kSaveDialogKey{"SaveGraphDlg"};
  static constexpr auto kLoadDialogKey{"LoadGraphDlg"};
  static constexpr auto kFileFilter{"JSON files{.json},All files{.*}"};

  std::atomic<bool> is_time_flowing{true};
};


#endif // NAG_EDITOR_GRAPH_EDITOR_UI_H
