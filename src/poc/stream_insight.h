#ifndef NAG_STREAM_INSIGHT_H
#define NAG_STREAM_INSIGHT_H

#include "editor/ui_window.h"

class StreamInsight : public UIWindow {

public:
  StreamInsight(): UIWindow("Stream Insight", 1280, 720) {
  }

protected:
  void render_ui() override;
};

#endif //NAG_STREAM_INSIGHT_H
