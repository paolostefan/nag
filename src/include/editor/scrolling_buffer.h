#ifndef NAG_SCROLLING_BUFFER_H
#define NAG_SCROLLING_BUFFER_H

#include "imgui.h"

// Utility structure for realtime plot
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

#endif //NAG_SCROLLING_BUFFER_H