#ifndef NAG_NODE_INSIGHT_H
#define NAG_NODE_INSIGHT_H

#include "editor/ui_window.h"
#include "engine/node.h"

class NodeInsight : public UIWindow {

public:
  NodeInsight();

protected:
  void render_ui() override;
  void evaluate_node_graph() const;

  Stream<float> time_stream;
  Stream<float> sin_a_stream_out;
  Stream<float> sin_b_stream_out;

  SinNode sin_a;
  SinNode sin_b;

  Stream<float> out_stream;

  AddFloatNode adding_node;

  std::vector<Node *> graph;
};


#endif //NAG_NODE_INSIGHT_H