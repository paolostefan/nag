#ifndef NAG_ENGINE_RENDER_TARGET_H
#define NAG_ENGINE_RENDER_TARGET_H


#include "GL/glew.h"


class RenderTarget {
public:
  RenderTarget() = default;

  ~RenderTarget() {
    cleanup();
  }

  /**
   * Initialize FBO with specified dimensions.
   * @param width Texture width
   * @param height Texture height
   * @param with_depth_buffer Create depth buffer attachment
   * @return true if successful
   */
  bool initialize(int width, int height, bool with_depth_buffer = false);

  /**
   * Bind this FBO for rendering.
   */
  void bind() const {
    if (initialized_) {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
      glViewport(0, 0, width_, height_);
    }
  }

  /**
   * Unbind FBO (return to default framebuffer).
   */
  static void unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  /**
   * Clear the render target.
   */
  void clear(const float r = 0.f, const float g = 0.f, const float b = 0.f, const float a = 0.f) const {
    if (initialized_) {
      bind();
      glClearColor(r, g, b, a);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
  }

  /**
   * Resize the render target.
   */
  bool resize(const int width, const int height) {
    if (width == width_ && height == height_) {
      return true;
    }
    return initialize(width, height, depth_buffer_ != 0);
  }

  [[nodiscard]] constexpr GLuint get_texture() const { return texture_; }
  [[nodiscard]] constexpr GLuint get_fbo() const { return fbo_; }
  [[nodiscard]] constexpr int get_width() const { return width_; }
  [[nodiscard]] constexpr int get_height() const { return height_; }
  [[nodiscard]] constexpr float get_fwidth() const { return static_cast<float>(width_); }
  [[nodiscard]] constexpr float get_fheight() const { return static_cast<float>(height_); }
  [[nodiscard]] constexpr bool is_valid() const { return initialized_; }

private:
  void cleanup();

  GLuint texture_{0};
  GLuint fbo_{0};
  GLuint depth_buffer_{0};
  int width_{0};
  int height_{0};
  bool initialized_{false};
};

/**
 *  Texture wrapper for stream output
 *  Holds reference to RenderTarget's texture.
 */
struct Texture {
  GLuint texture_id{0};
  int width{0};
  int height{0};

  Texture() = default;

  Texture(GLuint id, int w, int h) : texture_id(id), width(w), height(h) {
  }

  [[nodiscard]] constexpr bool is_valid() const { return texture_id != 0; }
};

#endif //NAG_ENGINE_RENDER_TARGET_H
