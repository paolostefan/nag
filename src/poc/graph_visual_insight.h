#ifndef NAG_GRAPH_INSIGHT_H
#define NAG_GRAPH_INSIGHT_H

#include "imnodes.h"

#include "editor/ui_window.h"
#include "engine/generator_nodes.h"
#include "engine/node_graph.h"
#include "engine/visual_nodes.h"
#include "engine/serialization/json_graph_serializer.h"


class GraphVisualInsight : public UIWindow {
public:
  void build_default_graph();

  GraphVisualInsight();

protected:
  void render_ui() override;

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
   * @brief Clears the graph and resets all dependent state.
   *
   * Invalidates stream pointers and resets the ImNodes first-render flag.
   */
  void reset_graph();

  /**
   * @brief Saves the current graph to the given path.
   *
   * @param path Absolute or relative path to the output JSON file
   */
  void save_graph(const std::string &path);

  /**
   * @brief Loads a graph from the given path, replacing the current one.
   *
   * @param path Absolute or relative path to the JSON file
   */
  void load_graph(const std::string &path);

  /**
   * @brief Spawns a new node of the given type at a canvas position.
   *
   * @param type     Node type to create via registry
   * @param position Screen-space position for the new node
   */
  void spawn_node(NodeType type, const ImVec2 &position);

  // ── Graph State ───────────────────────────────────────────────────────────
  NodeGraph graph;
  TimeNode *time_node{};

  Stream<float> *noise_stream_out{};
  Stream<float> *sin_a_stream_out{};
  Stream<float> *sin_b_stream_out{};
  Stream<float> *out_stream{};

  // ── ImNodes State ─────────────────────────────────────────────────────────
  ImNodesEditorContext *editor_context{nullptr};

  /** True only on the first render after a graph change (load/reset). */
  bool first_render{true};

  // ── UI State ──────────────────────────────────────────────────────────────

  /** Controls plot time flow. */
  bool plot_flowing{true};

  float plot_history{10.0f};

  /**
   * @brief Feedback message shown after save/load operations.
   *
   * Empty string means no message to show.
   */
  std::string status_message;

  /** Timestamp (ImGui time) when status_message was set. */
  float status_message_time{0.0f};

  static constexpr float kStatusMessageDuration{3.0f}; // seconds
  static constexpr auto kSaveDialogKey{"SaveGraphDlg"};
  static constexpr auto kLoadDialogKey{"LoadGraphDlg"};
  static constexpr auto kFileFilter{"JSON files{.json},All files{.*}"};

  // ── Serialization ─────────────────────────────────────────────────────────
  JsonGraphSerializer serializer;
};


#endif //NAG_GRAPH_INSIGHT_H
