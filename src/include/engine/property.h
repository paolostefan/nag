#ifndef NAG_ENGINE_PROPERTY_H
#define NAG_ENGINE_PROPERTY_H

#include <functional>
#include <string>
#include <variant>

#include "engine/visual_types.h"

enum class WidgetKind : uint8_t {
  SliderFloat,
  DragFloat,
  SliderInt,
  DragInt,
  InputInt,
  ColorEdit,
  Combo,
  Checkbox
};

enum class SliderFlag : uint8_t {
  None = 0,
  Logarithmic = 1
};

enum class ParamType : uint8_t {
  Float,
  Int,
  Bool,
  Vec4
};

using PropertyValue = std::variant<float, int, bool, Vec4>;

/// Describes a single node parameter. One declarative row drives:
///   - the editor widget (via the typed live `value` pointer),
///   - JSON serialization / deserialization (via `get` / `set`),
///   - `get_param(name)` float lookups.
///
/// Composite values (Vec4 arrays like `"color": [r,g,b,a]`) are declared as
/// ColorEdit rows for widget purposes but are serialized by per-node hooks;
/// the base schema loop skips rows whose `kind == WidgetKind::ColorEdit`.
struct Property {
  /// Machine name — JSON key, get_param name, undo param_name.
  std::string name;
  /// Display label for the properties panel.
  std::string label;
  WidgetKind kind{};
  ParamType param_type{};
  float min{0.f};
  float max{0.f};
  float speed{0.1f};
  const char *format{nullptr};
  /// Combo item labels (must outlive the Property).
  const char *const *items{nullptr};
  int item_count{0};
  /// Pin name; widget is disabled (and emits no commands) while connected.
  std::string disable_pin;
  SliderFlag slider_flags{SliderFlag::None};
  /// Live value buffer for 3-phase editor widgets (points into the node field).
  std::variant<float *, int *, bool *, Vec4 *> value;
  /// Serialization read.
  std::function<PropertyValue()> get;
  /// Serialization write / undo-setter (writes the node field).
  std::function<void(PropertyValue)> set;
};

template <typename NodeT>
Property MakeFloatProp(NodeT &node, float NodeT::*field,
                       const char *name, const char *label,
                       WidgetKind kind, float min, float max,
                       const char *format = nullptr,
                       SliderFlag flags = SliderFlag::None) {
  Property p;
  p.name = name;
  p.label = label;
  p.kind = kind;
  p.param_type = ParamType::Float;
  p.min = min;
  p.max = max;
  p.format = format;
  p.slider_flags = flags;
  p.value = &(node.*field);
  p.get = [&node, field]() -> PropertyValue { return node.*field; };
  p.set = [&node, field](PropertyValue v) { node.*field = std::get<float>(v); };
  return p;
}

template <typename NodeT>
Property MakeIntProp(NodeT &node, int NodeT::*field,
                     const char *name, const char *label,
                     WidgetKind kind, float min = 0.f, float max = 0.f,
                     const char *format = nullptr) {
  Property p;
  p.name = name;
  p.label = label;
  p.kind = kind;
  p.param_type = ParamType::Int;
  p.min = min;
  p.max = max;
  p.format = format;
  p.value = &(node.*field);
  p.get = [&node, field]() -> PropertyValue { return node.*field; };
  p.set = [&node, field](PropertyValue v) { node.*field = std::get<int>(v); };
  return p;
}

template <typename NodeT>
Property MakeBoolProp(NodeT &node, bool NodeT::*field,
                      const char *name, const char *label,
                      WidgetKind kind = WidgetKind::Checkbox) {
  Property p;
  p.name = name;
  p.label = label;
  p.kind = kind;
  p.param_type = ParamType::Bool;
  p.value = &(node.*field);
  p.get = [&node, field]() -> PropertyValue { return node.*field; };
  p.set = [&node, field](PropertyValue v) { node.*field = std::get<bool>(v); };
  return p;
}

template <typename NodeT, typename EnumT>
Property MakeEnumProp(NodeT &node, EnumT NodeT::*field,
                      const char *name, const char *label,
                      const char *const *items, int item_count) {
  Property p;
  p.name = name;
  p.label = label;
  p.kind = WidgetKind::Combo;
  p.param_type = ParamType::Int;
  p.items = items;
  p.item_count = item_count;
  p.value = reinterpret_cast<int *>(&(node.*field));
  p.get = [&node, field]() -> PropertyValue { return static_cast<int>(node.*field); };
  p.set = [&node, field](PropertyValue v) { node.*field = static_cast<EnumT>(std::get<int>(v)); };
  return p;
}

template <typename NodeT>
Property MakeColorProp(NodeT &node, Vec4 NodeT::*field,
                       const char *name, const char *label) {
  Property p;
  p.name = name;
  p.label = label;
  p.kind = WidgetKind::ColorEdit;
  p.param_type = ParamType::Vec4;
  p.value = &(node.*field);
  p.get = [&node, field]() -> PropertyValue { return node.*field; };
  p.set = [&node, field](PropertyValue v) { node.*field = std::get<Vec4>(v); };
  return p;
}

#endif // NAG_ENGINE_PROPERTY_H