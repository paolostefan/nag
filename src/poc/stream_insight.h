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

// utility structure for realtime plot
struct ScrollingBuffer {
  int MaxSize;
  int Offset;
  ImVector<ImVec2> Data;

  explicit ScrollingBuffer(const int max_size = 2000) {
    MaxSize = max_size;
    Offset  = 0;
    Data.reserve(MaxSize);
  }

  void AddPoint(const float x, const float y) {
    if (Data.size() < MaxSize)
      Data.push_back(ImVec2(x,y));
    else {
      Data[Offset] = ImVec2(x,y);
      Offset =  (Offset + 1) % MaxSize;
    }
  }

  void Erase() {
    if (!Data.empty()) {
      Data.shrink(0);
      Offset  = 0;
    }
  }
};


#endif //NAG_STREAM_INSIGHT_H
