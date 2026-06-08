#include "engine/render_target.h"

#include "spdlog/spdlog.h"

bool RenderTarget::initialize(const int width,
                              const int height,
                              const bool with_depth_buffer) {
  if (fbo_ != 0) {
    cleanup();
  }

  width_ = width;
  height_ = height;

  // Create texture
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
               width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Create FBO
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         texture_,
                         0);

  // Optional depth buffer
  if (with_depth_buffer) {
    glGenRenderbuffers(1, &depth_buffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              depth_buffer_);
  }

  // Check FBO status
  if (const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER); status != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("Framebuffer incomplete: {}", status);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    cleanup();
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  initialized_ = true;

  spdlog::debug("RenderTarget initialized: {}x{}", width, height);
  return true;
}

void RenderTarget::free_texture() {
  if (texture_ != 0) {
    glDeleteTextures(1, &texture_);
    texture_ = 0;
  }
}

void RenderTarget::cleanup() {
  free_texture();

  if (fbo_ != 0) {
    glDeleteFramebuffers(1, &fbo_);
    fbo_ = 0;
  }
  if (depth_buffer_ != 0) {
    glDeleteRenderbuffers(1, &depth_buffer_);
    depth_buffer_ = 0;
  }
  initialized_ = false;
}
