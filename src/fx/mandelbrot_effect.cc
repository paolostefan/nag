#include "fx/mandelbrot_effect.h"

MandelbrotEffect::MandelbrotEffect()
{
  shader = std::make_shared<ShaderProgram>();
  shader->load_from_files("shaders/mandelbrot.vert", "shaders/mandelbrot.frag");
}