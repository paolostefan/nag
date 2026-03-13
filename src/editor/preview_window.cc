#include "editor/preview_window.h"

#include "spdlog/spdlog.h"

// ── Shader sources ────────────────────────────────────────────────────────────

static auto kVertexShaderSrc = R"glsl(
#version 330 core

// Generates a fullscreen quad from gl_VertexID — no VBO needed.
// Vertices:  0,1,2  (lower-left triangle)
//            3,4,5  (upper-right triangle)
out vec2 v_uv;

void main() {
    // Two-triangle strip covering clip space [-1, 1].
    const vec2 kPositions[6] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0,  1.0)
    );

    vec2 pos = kPositions[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);

    // Map [-1,1] → [0,1] and flip Y: OpenGL FBO origin is bottom-left,
    // but we want top-left to match screen conventions.
    v_uv = vec2(pos.x * 0.5 + 0.5, 1.0 - (pos.y * 0.5 + 0.5));
}
)glsl";

static auto kFragmentShaderSrc = R"glsl(
#version 330 core

in  vec2      v_uv;
out vec4      frag_color;
uniform sampler2D u_texture;

void main() {
    frag_color = texture(u_texture, v_uv);
}
)glsl";

// ── PreviewWindow::Open ───────────────────────────────────────────────────────

bool PreviewWindow::open(SDL_Window *parent_window, SDL_GLContext gl_context) {
  if (window_) return true; // Already open.

  // Position the preview window next to the parent.
  int parent_x = 0, parent_y = 0;
  SDL_GetWindowPosition(parent_window, &parent_x, &parent_y);

  // Default size matches the current default RenderTarget resolution.
  // The window is resizable so the user can scale it freely.
  constexpr int kDefaultWidth = 400;
  constexpr int kDefaultHeight = 300;

  window_ = SDL_CreateWindow(
    "Preview",
    parent_x + 40, parent_y + 40,
    kDefaultWidth, kDefaultHeight,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
  );

  if (!window_) {
    spdlog::error("PreviewWindow: SDL_CreateWindow failed: {}", SDL_GetError());
    return false;
  }

  // Switch to the preview window to load the shader,
  // then give the context back to the parent window.
  SDL_GL_MakeCurrent(window_, gl_context);

  shader_program_ = ShaderManager::instance().load_from_source("preview_blit", kVertexShaderSrc, kFragmentShaderSrc);
  if (!shader_program_ || !shader_program_->is_valid()) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    SDL_GL_MakeCurrent(parent_window, gl_context);
    return false;
  }

  // Create an empty VAO for rendering with gl_VertexID.
  glGenVertexArrays(1, &vao_);

  SDL_GL_MakeCurrent(parent_window, gl_context);

  paused_ = false;
  spdlog::info("PreviewWindow opened ({}x{})", kDefaultWidth, kDefaultHeight);
  return true;
}

// ── PreviewWindow::close ──────────────────────────────────────────────────────

void PreviewWindow::close() {
  if (!window_) return;

  // Make the preview window context current to delete GL resources.
  SDL_GL_MakeCurrent(window_, nullptr);

  // GL resources are managed by ShaderManager, no need to delete shader.
  shader_program_ = nullptr;

  if (vao_) {
    glDeleteVertexArrays(1, &vao_);
    vao_ = 0;
  }

  SDL_DestroyWindow(window_);
  window_ = nullptr;

  spdlog::info("PreviewWindow closed");
}

// ── PreviewWindow::Render ─────────────────────────────────────────────────────

void PreviewWindow::render(const Texture *texture,
                           SDL_GLContext gl_context,
                           SDL_Window *return_to) const {
  if (!is_open() || !texture || !texture->is_valid()) return;

  // ── 1. Switch context to the preview window ────────────────────────────────
  SDL_GL_MakeCurrent(window_, gl_context);

  // ── 2. Viewport matches the window's current drawable size ────────────────
  //  (the window is resizable, so query SDL every frame)
  int w = 0, h = 0;
  SDL_GL_GetDrawableSize(window_, &w, &h);
  glViewport(0, 0, w, h);

  // ── 3. Clear + draw ────────────────────────────────────────────────────────
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  shader_program_->use();

  // Bind the texture to unit 0 and set the uniform.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
  shader_program_->set_uniform("u_texture", 0);

  // Draw the fullscreen quad using gl_VertexID.
  glBindVertexArray(vao_);
  FullscreenQuadRenderer::instance().render();
  glBindVertexArray(0);

  ShaderProgram::unuse();

  // ── 4. Present + restore context ──────────────────────────────────────────
  SDL_GL_SwapWindow(window_);
  SDL_GL_MakeCurrent(return_to, gl_context);
}
