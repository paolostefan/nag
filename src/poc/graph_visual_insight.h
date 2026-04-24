#ifndef NAG_GRAPH_INSIGHT_H
#define NAG_GRAPH_INSIGHT_H

#include "imnodes.h"
#include "editor/node_properties_panel.h"

#include "editor/ui_window.h"
#include "editor/graph_editor.h"
#include "editor/preview_window.h"
#include "engine/nodes/generator_nodes.h"
#include "engine/nodes/visual_node.h"


class GraphVisualInsight : public UIWindow, public GraphEditor {
public:
  GraphVisualInsight();

protected:
  void build_default_graph() override;

  void main_event_loop() override;

  void render_ui() override;

  void reset_graph() override;

private:
  // ── Rendering ─────────────────────────────────────────────────────────────
  void render_menu_bar();

  void render_plot();

  void render_node_editor();

  void render_context_menu();

  static void render_visual_node_body(const VisualNode *visual_node);

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
   * that reference the deleted nodes. Invalidates stream pointers
   * if any deleted node was being plotted.
   */
  void delete_selected_nodes();

  /**
   * @brief Deletes selected links.
   */
  void delete_selected_links();

  // ── Graph State ───────────────────────────────────────────────────────────

  PreviewWindow preview_window;

  Stream<float> *noise_stream_out{};
  Stream<float> *sin_a_stream_out{};
  Stream<float> *sin_b_stream_out{};
  Stream<float> *out_stream{};

  // ── ImNodes State ─────────────────────────────────────────────────────────
  ImNodesEditorContext *editor_context{nullptr};

  // ── UI State ──────────────────────────────────────────────────────────────

  /** Controls plot time flow. */
  bool plot_flowing{true};

  float plot_history{10.f};

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
  static constexpr auto kSaveGraphDialogKey{"SaveGraphDlg"};
  static constexpr auto kLoadGraphDialogKey{"LoadGraphDlg"};
  static constexpr auto kFileFilter{"JSON files{.json},All files{.*}"};
};


#endif //NAG_GRAPH_INSIGHT_H
