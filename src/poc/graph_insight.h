#ifndef NAG_GRAPH_INSIGHT_H
#define NAG_GRAPH_INSIGHT_H

#include "editor/ui_window.h"
#include "engine/nodes/generator_nodes.h"
#include "engine/nodes/math_nodes.h"
#include "engine/node_graph.h"

class GraphInsight : public UIWindow {
public:
  GraphInsight();

protected:
  void render_ui() override;

  NodeGraph graph;
  Stream *sin_a_stream_out{};
  Stream *sin_b_stream_out{};
  Stream *out_stream{};

  TimeNode *time_node{};

  SinNode *sin_a{};
  CosNode *sin_b{};
  AddNode *adding_node{};
};


#endif //NAG_GRAPH_INSIGHT_H
