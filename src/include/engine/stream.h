#ifndef NAG_STREAM_H
#define NAG_STREAM_H

#include <cmath>
#include <typeinfo>

/**
 * Base class for streams.
 *
 * Today I've learned a new pattern name: "type erasure".
 */
struct StreamBase {
  virtual ~StreamBase() = default;

  [[nodiscard]] virtual const std::type_info &type() const = 0;

  [[nodiscard]] virtual const char *name() const = 0;
};

template<typename T>
struct Stream : public StreamBase {

  // Default implementation for at(): return the current value ignoring time.
  virtual T at(double time) { return value; };

  [[nodiscard]] const std::type_info &type() const override { return typeid(T); }
  [[nodiscard]] const char *name() const override { return typeid(T).name(); }

  T value{};
};

struct SinStream : Stream<float> {

  double frequency = 1.0;
  double amplitude = 1.0;
  double offset = 0.0;

  explicit SinStream(const double frequency = 1.0, const double amplitude = 1.0, const double offset = 0.0)
      : frequency(frequency), amplitude(amplitude), offset(offset) {}

  float at(const double time) override {
    return value = static_cast<float>(amplitude * std::sin(time * frequency + offset));
  }
};

#endif //NAG_STREAM_H
