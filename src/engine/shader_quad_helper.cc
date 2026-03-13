#include "engine/shader_quad_helper.h"

void ShaderQuadHelper::initialize() {
  if (initialized) return;

  // Create an empty VAO — required for core profile OpenGL even with gl_VertexID.
  glGenVertexArrays(1, &vao);

  initialized = true;
}

void ShaderQuadHelper::render() const {
  if (!initialized) return;

  // Bind the VAO before drawing.
  glBindVertexArray(vao);
  FullscreenQuadRenderer::instance().render();
  glBindVertexArray(0);
}

void ShaderQuadHelper::cleanup() {
  if (!initialized) return;

  if (vao) {
    glDeleteVertexArrays(1, &vao);
    vao = 0;
  }

  initialized = false;
}
