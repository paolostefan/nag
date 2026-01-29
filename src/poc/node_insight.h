#ifndef NAG_NODE_INSIGHT_H
#define NAG_NODE_INSIGHT_H

#include "editor/ui_window.h"

class NodeInsight : public UIWindow {

public:
  NodeInsight(): UIWindow("Node Insight", 1280, 720) {
  }

protected:
  void render_ui() override;
};


#endif //NAG_NODE_INSIGHT_H