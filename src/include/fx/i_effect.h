#ifndef NAG_ENGINE_INTERFACE_EFFECT_H
#define NAG_ENGINE_INTERFACE_EFFECT_H

#include <string>
#include <vector>

#include "fx/effect_parameter.h"

/**
 * @brief Interface for effects
 */
class IEffect
{
public:
  virtual ~IEffect() = default;

  virtual std::string get_name() const = 0;
  virtual std::string get_description() const { return ""; }

  virtual bool initialize() = 0;
  virtual void cleanup() = 0;

  virtual void render(int width, int height, double time_ms) = 0;

  virtual std::vector<EffectParameter> &get_parameters() = 0;

  /**
   * @brief Helper function to get a parameter by name
   */
  inline EffectParameter *get_parameter(const std::string &name)
  {
    auto &params = get_parameters();

    for (auto &param : params)
    {
      if (param.get_name() == name)
      {
        return &param;
      }
    }

    return nullptr;
  }

  void update_parameters(const double time_ms)
  {
    auto &params = get_parameters();

    for (auto &param : params)
    {
      param.set_value(param.evaluate(time_ms));
    }
  }

protected:
  IEffect() = default;
};

#endif // NAG_ENGINE_INTERFACE_EFFECT_H