#ifndef NAG_STREAM_H
#define NAG_STREAM_H

#include <cmath>

template<typename T>
struct Stream {
  virtual ~Stream() = default;

  virtual T at(double time) = 0;
};

struct SinStream : Stream<float> {
  float at(const double time) override {
    return std::sin(static_cast<float>(time));
  }
};

#endif //NAG_STREAM_H
