#include "fx/mandelbrot_effect.h"

#include "spdlog/spdlog.h"

MandelbrotEffect::MandelbrotEffect() {
  // Initialize parameters with default values
  parameters.emplace_back("zoom", ParameterType::FLOAT, 1.f, 0.1f, 1000.f, 0.1f);
  parameters.emplace_back("center", ParameterType::VEC2, Vec2{-0.5f, 0.f}, Vec2{-2.f, -2.f}, Vec2{2.f, 2.f},
                          Vec2{.0001f, .0001f}, "%.6f");
  parameters.emplace_back("iterations", ParameterType::INT, 100, 10, 1000, 1);
  parameters.emplace_back("color_offset", ParameterType::FLOAT, 0.f, -1.f, 1.f, 0.01f);
}

MandelbrotEffect::~MandelbrotEffect() {
  MandelbrotEffect::cleanup();
}

bool MandelbrotEffect::initialize() {
  // Load shader
  shader = ShaderManager::instance().load(
    "mandelbrot",
    "shaders/fullscreen_quad.vert",
    "shaders/mandelbrot.frag");

  if (!shader || !shader->is_valid()) {
    spdlog::error("Failed to load Mandelbrot shader");
    return false;
  }

  setup_quad();

  spdlog::info("Mandelbrot effect initialized");
  return true;
}

void MandelbrotEffect::cleanup() {
  if (vao) {
    glDeleteVertexArrays(1, &vao);
    vao = 0;
  }
  if (vbo) {
    glDeleteBuffers(1, &vbo);
    vbo = 0;
  }
}

void MandelbrotEffect::render(const int width, const int height, const double time_ms) {
  if (!shader || !shader->is_valid())
    return;

  // Update parameters based on keyframes
  update_parameters(time_ms);

  shader->use();

  // Set resolution
  shader->set_uniform("u_resolution", static_cast<float>(width), static_cast<float>(height));

  // Set parameters from current values
  if (const auto *zoom_param = get_parameter("zoom")) {
    const float zoom = std::get<float>(zoom_param->get_value());
    shader->set_uniform("u_zoom", zoom);
  }

  if (const auto *center_param = get_parameter("center")) {
    const auto [x, y] = std::get<Vec2>(center_param->get_value());
    shader->set_uniform("u_center", x, y);
  }

  if (const auto *iter_param = get_parameter("iterations")) {
    const int iterations = std::get<int>(iter_param->get_value());
    shader->set_uniform("u_iterations", iterations);
  }

  if (const auto *color_param = get_parameter("color_offset")) {
    const float color_offset = std::get<float>(color_param->get_value());
    shader->set_uniform("u_color_offset", color_offset);
  }

  // Render fullscreen quad
  render_quad();

  shader->unuse();
}

void MandelbrotEffect::setup_quad() {
  // Fullscreen quad vertices (position + texcoords)
  constexpr float vertices[] = {
    // pos (x, y)   // texcoord (u, v)
    -1.f,
    -1.f,
    0.f,
    0.f,
    1.f,
    -1.f,
    1.f,
    0.f,
    1.f,
    1.f,
    1.f,
    1.f,

    -1.f,
    -1.f,
    0.f,
    0.f,
    1.f,
    1.f,
    1.f,
    1.f,
    -1.f,
    1.f,
    0.f,
    1.f,
  };

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute (location = 0)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Texcoord attribute (location = 1)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void MandelbrotEffect::render_quad() const {
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}
