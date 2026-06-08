#ifndef NAG_ENGINE_EFFECT_PARAMETER_H
#define NAG_ENGINE_EFFECT_PARAMETER_H

#include <stdexcept>
#include <string>
#include <vector>
#include <variant>

#include "engine/visual_types.h"

enum class ParameterType
{
  FLOAT,
  VEC2,
  VEC3,
  VEC4,
  COLOR_RGB,
  COLOR_RGBA,
  INT
};

using ParameterValue = std::variant<float, Vec2, Vec3, Vec4, int>;

struct Keyframe
{
  double time_ms{.0};
  ParameterValue value;

  bool operator<(const Keyframe &other) const
  {
    return time_ms < other.time_ms;
  }
};

enum class InterpolationType
{
  STEP,
  LINEAR,
  // Future: BEZIER, EASE_IN, EASE_OUT, etc.
};

class EffectParameter
{
public:
  EffectParameter(const std::string &name,
                  ParameterType type,
                  ParameterValue default_value, 
                  ParameterValue min_value,
                  ParameterValue max_value,
                  ParameterValue step,
                  const std::string &format_string = ""
                )
      : name(name),
        type(type),
        current_value(default_value),
        default_value(default_value),
        min_value(min_value),
        max_value(max_value),
        step(step),
        fmt(format_string)
  {
  }

  const std::string &get_name() const { return name; }
  const std::string &get_fmt() const { return fmt; }
  
  [[nodiscard]] constexpr ParameterType get_type() const { return type; }

  [[nodiscard]] constexpr InterpolationType get_interpolation() const { return interpolation; }
  void set_interpolation(const InterpolationType interp) { interpolation = interp; }

  [[nodiscard]] const ParameterValue &get_value() const { return current_value; }
  void set_value(const ParameterValue &value) { current_value = value; }
  
  [[nodiscard]] const ParameterValue &get_min_value() const { return min_value; }
  [[nodiscard]] const ParameterValue &get_max_value() const { return max_value; }
  [[nodiscard]] const ParameterValue &get_step() const { return step; }

  void add_keyframe(const double time_ms, const ParameterValue &value)
  {
    keyframes.emplace_back(time_ms, value);
  }
  
  void remove_keyframe(const size_t index) noexcept
  {
    if (index < keyframes.size())
      keyframes.erase(keyframes.begin() + index);
  }

  void clear_keyframes() noexcept { keyframes.clear(); }

  [[nodiscard]] const std::vector<Keyframe> &get_keyframes() const { return keyframes; }

  // Evaluate parameter value at given time with interpolation
  [[nodiscard]] ParameterValue evaluate(double time_ms, InterpolationType interp = InterpolationType::LINEAR) const{
    if(keyframes.empty())
      return current_value;

    throw std::runtime_error("Not implemented");
  }

private:
  /// @brief Short parameter name, used in IDs and JSON, MUST NOT contain spaces or special chars.
  std::string name;

  /// @brief Format string for displaying the parameter in UI
  std::string fmt{""};

  ParameterType type;
  ParameterValue current_value;
  ParameterValue default_value;
  ParameterValue min_value;
  ParameterValue max_value;

  /// @brief Step size for value changes (used in UI)
  ParameterValue step;

  std::vector<Keyframe> keyframes;

  InterpolationType interpolation{InterpolationType::LINEAR};

  ParameterValue interpolate_linear(const ParameterValue &a, const ParameterValue &b, float t) const;
};

#endif // NAG_ENGINE_EFFECT_PARAMETER_H