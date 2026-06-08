#ifndef NAG_STREAM_H
#define NAG_STREAM_H

#include <cstdint>
#include <memory>
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

  // Former dirty marker, evolved for fan-out
  uint64_t version{0};
};

template<typename T>
struct Stream : StreamBase {
  [[nodiscard]] const std::type_info &type() const override { return typeid(T); }
  [[nodiscard]] const char *name() const override { return typeid(T).name(); }

  T value{};

  explicit Stream(const T start_value = T{}) {
    value = start_value;
  }

  constexpr void update(T val) {
    value = val;
    version++;
  }

  /**
   * Create an external stream not connected to any node.
   * Useful for streams like time_stream that are updated externally.
   *
   * @tparam T Type of the stream value
   * @param initial_value Initial value for the stream
   * @return Pointer to the created stream
   */
  static std::unique_ptr<Stream> create(const T &initial_value = T{}) {
    return std::make_unique<Stream>(initial_value);
  }
};


#endif //NAG_STREAM_H
