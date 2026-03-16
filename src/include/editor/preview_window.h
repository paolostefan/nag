#ifndef NAG_EDITOR_PREVIEW_WINDOW_H
#define NAG_EDITOR_PREVIEW_WINDOW_H

#include "GL/glew.h"
#include "SDL.h"
#include "engine/render_target.h"
#include "engine/shader_manager.h"

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

  ~PreviewWindow() { close(); }

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
  bool open(SDL_Window *parent_window, SDL_GLContext gl_context);

  /**
   * @brief Destroys the SDL_Window and releases GL resources.
   * Safe to call multiple times.
   */
  void close();

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
  void render(const Texture *texture,
              SDL_GLContext gl_context,
              SDL_Window *return_to) const;

  [[nodiscard]] bool is_open() const { return window_ != nullptr; }
  [[nodiscard]] bool is_paused() const { return paused_; }
  void toggle_pause() { paused_ = !paused_; }

  /**
   * @brief Returns the SDL window ID, used to match SDL_WINDOWEVENT_CLOSE.
   * Returns 0 if the window is not open.
   */
  [[nodiscard]] Uint32 GetWindowID() const {
    return window_ ? SDL_GetWindowID(window_) : 0;
  }

private:
  SDL_Window *window_{nullptr};
  std::shared_ptr<ShaderProgram> shader_program_{nullptr};
  GLuint vao_{0}; ///< Empty VAO required by core profile for gl_VertexID.
  bool paused_{false};
};


#endif //NAG_EDITOR_PREVIEW_WINDOW_H
