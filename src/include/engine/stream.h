#ifndef NAG_STREAM_H
#define NAG_STREAM_H

#include <cstdint>
#include <memory>
#include <variant>

#include "engine/data_type.h"
#include "engine/particles2d.h"

struct Texture;

struct StreamBase {
  virtual ~StreamBase() = default;

  uint64_t version{0};
};

using StreamValue = std::variant<std::monostate, float, bool, Texture *, Particles2D>;

struct Stream : StreamBase {
  StreamValue value;

  explicit Stream(const StreamValue v = {}) : value(v) {}

  [[nodiscard]] float *as_float() { return std::get_if<float>(&value); }
  [[nodiscard]] const float *as_float() const { return std::get_if<float>(&value); }
  [[nodiscard]] bool *as_bool() { return std::get_if<bool>(&value); }
  [[nodiscard]] const bool *as_bool() const { return std::get_if<bool>(&value); }
  [[nodiscard]] Texture **as_texture() { return std::get_if<Texture *>(&value); }
  [[nodiscard]] Texture *const *as_texture() const { return std::get_if<Texture *>(&value); }
  [[nodiscard]] Particles2D *as_particles() { return std::get_if<Particles2D>(&value); }
  [[nodiscard]] const Particles2D *as_particles() const { return std::get_if<Particles2D>(&value); }

  void update_float(const float v) { value = v; version++; }
  void update_bool(const bool v) { value = v; version++; }
  void update_texture(Texture *const v) { value = v; version++; }
  void update_particles(const Particles2D &v) { value = v; version++; }

  static std::unique_ptr<Stream> create(const StreamValue v = {}) {
    return std::make_unique<Stream>(v);
  }
};

inline StreamValue default_stream_value(const DataType t) {
  switch (t) {
    case DataType::Float: return 0.0f;
    case DataType::Bool: return false;
    case DataType::Texture: return static_cast<Texture *>(nullptr);
    case DataType::Particles2D: return Particles2D{};
    default: return std::monostate{};
  }
}

#endif
