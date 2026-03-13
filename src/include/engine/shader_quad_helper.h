#ifndef NAG_ENGINE_SHADER_QUAD_HELPER_H
#define NAG_ENGINE_SHADER_QUAD_HELPER_H


#include <GL/glew.h>
#include "engine/fullscreen_quad_renderer.h"

/**
 * Helper class for rendering fullscreen quads with shaders.
 * Reusable across all shader-based visual nodes.
 */
class ShaderQuadHelper {
public:
  // Delete copy constructor and assignment operator
  ShaderQuadHelper(const ShaderQuadHelper &) = delete;

  ShaderQuadHelper &operator=(const ShaderQuadHelper &) = delete;

  static ShaderQuadHelper &instance() {
    static ShaderQuadHelper instance;
    return instance;
  }

  /**
   * Initialize VAO/VBO for fullscreen quad.
   * Call once at startup.
   */
  void initialize();

  /**
   * Render the fullscreen quad.
   */
  void render() const;

  /**
   * Cleanup resources.
   */
  void cleanup();

  ~ShaderQuadHelper() {
    cleanup();
  }

private:
  ShaderQuadHelper() = default;

  GLuint vao{0};
  GLuint vbo{0};
  bool initialized{false};
};

#endif //NAG_ENGINE_SHADER_QUAD_HELPER_H
