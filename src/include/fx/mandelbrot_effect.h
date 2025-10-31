#ifndef NAG_ENGINE_MANDELBROT_EFFECT_H
#define NAG_ENGINE_MANDELBROT_EFFECT_H

#include "fx/i_effect.h"
#include "engine/shader_manager.h"

#include <memory>

class MandelbrotEffect : public IEffect
{
public:
  MandelbrotEffect();
  ~MandelbrotEffect() override;

  std::string get_name() const override { return "Mandelbrot"; }
  std::string get_description() const override { return "Classic Mandelbrot fractal"; }

  bool initialize() override;
  void cleanup() override;

  void render(int width, int height, double time_ms) override;

  std::vector<EffectParameter> &get_parameters() override { return parameters; }
  const std::vector<EffectParameter> &get_parameters() const { return parameters; }

private:
  std::shared_ptr<ShaderProgram> shader;
  std::vector<EffectParameter> parameters;

  GLuint vao{0};
  GLuint vbo{0};

  void setup_quad();
  void render_quad();
};

#endif // NAG_ENGINE_MANDELBROT_EFFECT_H