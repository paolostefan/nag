#ifndef NAG_STREAM_H
#define NAG_STREAM_H

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

#include "engine/data_type.h"
#include "engine/particles2d.h"

struct Texture;

struct StreamBase {
  virtual ~StreamBase() = default;

  uint64_t version{0};
};

using StreamValue = std::variant<std::monostate, float, int, bool, Texture *, Particles2D>;

struct Stream : StreamBase {
  StreamValue value;

  explicit Stream(StreamValue  v = {}) : value(std::move(v)) {}

  [[nodiscard]] float *as_float() { return std::get_if<float>(&value); }
  [[nodiscard]] const float *as_float() const { return std::get_if<float>(&value); }
  [[nodiscard]] int *as_int() { return std::get_if<int>(&value); }
  [[nodiscard]] const int *as_int() const { return std::get_if<int>(&value); }
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
    case DataType::Int: return 0;
    case DataType::Bool: return false;
    case DataType::Texture: return nullptr;
    case DataType::Particles2D: return Particles2D{};
    default: return std::monostate{};
  }
}

#endif
