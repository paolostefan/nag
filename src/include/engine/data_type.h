#ifndef NAG_ENGINE_DATA_TYPE_H
#define NAG_ENGINE_DATA_TYPE_H

enum class DataType : uint8_t {
  Float,
  Bool,
  Texture,
  Particles2D,

  Count
};

inline constexpr const char *data_type_name(const DataType t) {
  switch (t) {
    case DataType::Float: return "float";
    case DataType::Bool: return "bool";
    case DataType::Texture: return "Texture*";
    case DataType::Particles2D: return "Particles2D";
    default: return "unknown";
  }
}

#endif
