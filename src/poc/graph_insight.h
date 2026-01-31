#ifndef NAG_GRAPH_INSIGHT_H
#define NAG_GRAPH_INSIGHT_H

#include "editor/ui_window.h"
#include "engine/node_graph.h"

class GraphInsight : public UIWindow {
public:
  GraphInsight();

protected:
  void render_ui() override;

  NodeGraph graph;
  Stream<float> time_stream;
  Stream<float> sin_a_stream_out;
  Stream<float> sin_b_stream_out;
  Stream<float> out_stream;
  
  std::unique_ptr<SinNode> sin_a;
  std::unique_ptr<SinNode> sin_b;
  std::unique_ptr<AddFloatNode> adding_node;
};


#endif //NAG_GRAPH_INSIGHT_H
