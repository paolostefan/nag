#ifndef NAG_ENGINE_FULLSCREEN_QUAD_RENDERER_H
#define NAG_ENGINE_FULLSCREEN_QUAD_RENDERER_H

#include "GL/glew.h"

/**
 * Singleton class for rendering fullscreen quads using gl_VertexID.
 * Generates positions and texture coordinates from vertex ID for efficiency.
 * Replaces VBO-based quad rendering for consistency.
 */
class FullscreenQuadRenderer {
public:
  // Delete copy constructor and assignment operator
  FullscreenQuadRenderer(const FullscreenQuadRenderer &) = delete;
  FullscreenQuadRenderer &operator=(const FullscreenQuadRenderer &) = delete;

  static FullscreenQuadRenderer &instance() {
    static FullscreenQuadRenderer instance;
    return instance;
  }

  /**
   * Render the fullscreen quad.
   * Assumes a shader is already bound that uses gl_VertexID for positions.
   */
  static void render() {
    // Draw 6 vertices — the vertex shader generates positions from gl_VertexID.
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

private:
  FullscreenQuadRenderer() = default;
};

#endif // NAG_ENGINE_FULLSCREEN_QUAD_RENDERER_H
