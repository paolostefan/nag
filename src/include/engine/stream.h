#ifndef NAG_STREAM_H
#define NAG_STREAM_H

template<typename T>
class Stream {
public:
  virtual ~Stream() = default;
  virtual T at(double time) = 0;
};


#endif //NAG_STREAM_H