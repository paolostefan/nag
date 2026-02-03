#ifndef NAG_GRAPH_INSIGHT_H
#define NAG_GRAPH_INSIGHT_H

#include "imnodes.h"

#include "editor/ui_window.h"
#include "engine/node_graph.h"

class GraphVisualInsight : public UIWindow {
public:
  GraphVisualInsight();

protected:
  void render_ui() override;

  Stream<float> time_stream;
  Stream<float> sin_a_stream_out;
  Stream<float> sin_b_stream_out;
  Stream<float> out_stream;

  NodeGraph graph;

  ImNodesEditorContext *editor_context{nullptr};
};


#endif //NAG_GRAPH_INSIGHT_H
