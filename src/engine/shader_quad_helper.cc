#include "engine/shader_quad_helper.h"

void ShaderQuadHelper::initialize() {
  if (initialized) return;

  // Fullscreen quad vertices (position + texcoord)
  constexpr float vertices[] = {
    // pos (x, y)       // texcoord (u, v)
    -1.0f, -1.0f, /* */ 0.0f, 0.0f,
    1.0f, -1.0f, /*  */ 1.0f, 0.0f,
    1.0f, 1.0f, /*   */ 1.0f, 1.0f,

    -1.0f, -1.0f, /* */ 0.0f, 0.0f,
    1.0f, 1.0f, /*   */ 1.0f, 1.0f,
    -1.0f, 1.0f, /*  */ 0.0f, 1.0f,
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
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void *>(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  initialized = true;
}

void ShaderQuadHelper::render() const {
  if (!initialized) return;

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void ShaderQuadHelper::cleanup() {
  if (!initialized) return;

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  vao = 0;
  vbo = 0;
  initialized = false;
}
