#ifndef NAG_GRAPH_INSIGHT_H
#define NAG_GRAPH_INSIGHT_H

#include "imnodes.h"

#include "editor/ui_window.h"
#include "engine/generator_nodes.h"
#include "engine/node_graph.h"
#include "engine/visual_nodes.h"


class GraphVisualInsight : public UIWindow {
public:
  GraphVisualInsight();

protected:
  void render_ui() override;

  static void render_visual_node_body(const VisualNode * visual_node);

  static unsigned int get_pin_color(const Pin & pin);

  void render_node_editor();

  Stream<float> *noise_stream_out{};
  Stream<float> *sin_a_stream_out{};
  Stream<float> *sin_b_stream_out{};
  Stream<float> *out_stream{};

  TimeNode *time_node{};

  NodeGraph graph;

  ImNodesEditorContext *editor_context{nullptr};
};


#endif //NAG_GRAPH_INSIGHT_H
