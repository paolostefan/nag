#ifndef NAG_EDITOR_PREVIEW_WINDOW_H
#define NAG_EDITOR_PREVIEW_WINDOW_H

#include "GL/glew.h"
#include "SDL.h"
#include "engine/render_target.h"

/**
 * @brief Secondary SDL window that blits a Texture* to screen via a
 *        fullscreen-quad passthrough shader.
 *
 * Lifecycle:
 *   - Call Open() to create the SDL_Window and compile the shader.
 *   - Call Render() every frame (no-op if closed or texture invalid).
 *   - Call Close() when the user dismisses the window, or on shutdown.
 *
 * Threading: must be used from the same thread that owns the GL context.
 */
class PreviewWindow {
public:
  PreviewWindow() = default;

  ~PreviewWindow() { Close(); }

  // Non-copyable, non-movable (owns GL resources).
  PreviewWindow(const PreviewWindow &) = delete;

  PreviewWindow &operator=(const PreviewWindow &) = delete;

  /**
   * @brief Creates the SDL_Window and compiles the blit shader.
   * @param parent_window  The main SDL_Window (used only to position the new
   *                       window alongside it; the GL context is NOT borrowed
   *                       here — the caller must make it current beforehand).
   * @param gl_context     The shared GL context (must already be current on
   *                       the calling thread).
   * @return true on success.
   */
  bool Open(SDL_Window *parent_window, SDL_GLContext gl_context);

  /**
   * @brief Destroys the SDL_Window and releases GL resources.
   * Safe to call multiple times.
   */
  void Close();

  /**
   * @brief Blits @p texture to the preview window.
   *
   * Sequence:
   *   1. SDL_GL_MakeCurrent(preview_window_, gl_context)
   *   2. glViewport / clear / draw fullscreen quad
   *   3. SDL_GL_SwapWindow(preview_window_)
   *   4. SDL_GL_MakeCurrent(return_to, gl_context)   ← restores caller's ctx
   *
   * No-op if !IsOpen() or texture is nullptr / invalid.
   *
   * @param texture    The texture to display (non-owning).
   * @param gl_context The shared GL context.
   * @param return_to  The window to restore MakeCurrent on after the blit.
   */
  void Render(const Texture *texture,
              SDL_GLContext gl_context,
              SDL_Window *return_to);

  [[nodiscard]] bool IsOpen() const { return window_ != nullptr; }
  [[nodiscard]] bool IsPaused() const { return paused_; }
  void TogglePause() { paused_ = !paused_; }

  /**
   * @brief Returns the SDL window ID, used to match SDL_WINDOWEVENT_CLOSE.
   * Returns 0 if the window is not open.
   */
  [[nodiscard]] Uint32 GetWindowID() const {
    return window_ ? SDL_GetWindowID(window_) : 0;
  }

private:
  /** Compiles and links the blit shader. Returns program id or 0 on error. */
  [[nodiscard]] static GLuint CompileBlitShader();

  SDL_Window *window_{nullptr};
  GLuint shader_program_{0};
  GLuint vao_{0}; ///< Empty VAO required by core profile.
  bool paused_{false};
};


#endif //NAG_EDITOR_PREVIEW_WINDOW_H
