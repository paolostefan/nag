#ifndef NAG_STREAM_H
#define NAG_STREAM_H

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

  // Dirty marker, evolved for fan-out
  uint64_t version{0};
};

template<typename T>
struct Stream : StreamBase {

  [[nodiscard]] const std::type_info &type() const override { return typeid(T); }
  [[nodiscard]] const char *name() const override { return typeid(T).name(); }

  T value{};

  constexpr void update(T val) { value = val; version++; }
};

#endif //NAG_STREAM_H
