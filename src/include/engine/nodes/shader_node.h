#ifndef NAG_ENGINE_SHADER_NODE_H
#define NAG_ENGINE_SHADER_NODE_H

#include <string>

#include "GL/glew.h"

#include "engine/nodes/visual_node.h"
#include "engine/shader_manager.h"
#include "engine/shader_quad_helper.h"
#include "shaders/fullscreen_quad_vert.h"

/**
 * @brief Virtual base struct for post-process / effect nodes driven by a fragment shader.
 *
 * Extends VisualNode following the same pattern as GradientNode / CompositeNode:
 *   - initialize() loads the shader via ShaderManager (file-based, cached).
 *   - render()     binds Texture* input pins as samplers, float input pins as
 *                  float uniforms, draws the fullscreen quad.
 *
 * Subclasses must implement:
 *   - shader_name()      unique key for ShaderManager cache (e.g. "blur_gaussian")
 *   - frag_shader_src() path relative to working dir  (e.g. "shaders/blur.frag")
 *   - bind_params()      set node-specific uniforms after the base class has
 *                        already bound textures and float pins
 *
 * Uniform naming convention (bound automatically by render()):
 *   Texture* inputs  →  u_texture_0, u_texture_1, ...   (sampler2D)
 *   float    inputs  →  u_<pin.name>                     (float)
 *   always present   →  u_resolution                     (vec2)
 *
 * Two-pass nodes (e.g. separable blur) override render() and call
 * draw_pass() twice, using aux_target_ for the intermediate result.
 */
struct ShaderNode : VisualNode {
  std::shared_ptr<ShaderProgram> shader;

  // ── Interface for subclasses ──────────────────────────────────────────────

  /** Unique shader cache key, e.g. "blur_gaussian". */
  [[nodiscard]] virtual const char *shader_name() const noexcept = 0;

  /** Vertex shader source. Defaults to the shared fullscreen quad vertex shader. */
  [[nodiscard]] virtual const char *vert_shader_src() const noexcept {
    return kfullscreen_quad_vert;
  }

  /** Fragment shader source. */
  [[nodiscard]] virtual const char *frag_shader_src() const noexcept = 0;

  /**
   * @brief Called by render() after textures and float uniforms are bound.
   * Override to set additional node-specific uniforms (enums, vec2, etc.).
   */
  virtual void bind_params() {
  }

  virtual void update_from_inputs() {
  }

  // ── VisualNode overrides ──────────────────────────────────────────────────

  bool initialize(const int width, const int height) override {
    if (!VisualNode::initialize(width, height)) return false;

    shader = ShaderManager::instance().load(shader_name(),
                                            vert_shader_src(),
                                            frag_shader_src());
    return shader && shader->is_valid();
  }

  void render() override {
    if (!render_target || !render_target->is_valid() ||
        !shader || !shader->is_valid())
      return;

    update_from_inputs();

    render_target->clear();
    shader->use();

    bind_texture_inputs(shader.get());
    bind_float_inputs(shader.get());
    bind_params();
    shader->set_uniform("u_resolution",
                        render_target->get_fwidth(),
                        render_target->get_fheight());

    ShaderQuadHelper::instance().render();

    ShaderProgram::unuse();
    unbind_texture_inputs();
    RenderTarget::unbind();
  }

protected:
  /// Optional intermediate RenderTarget for two-pass effects (e.g. separable blur).
  std::unique_ptr<RenderTarget> aux_target_;

  /**
   * @brief Initializes aux_target_. Call from initialize() in two-pass nodes.
   */
  bool initialize_aux(const int width, const int height) {
    aux_target_ = std::make_unique<RenderTarget>();
    if (!aux_target_->initialize(width, height)) {
      spdlog::error("ShaderNode '{}': aux RenderTarget init failed", name);
      aux_target_.reset();
      return false;
    }
    return true;
  }

  /**
   * @brief Draws the fullscreen quad into @p target using @p prog.
   *
   * Used in the second pass of separable effects: @p tex_id_override replaces
   * pin-based texture binding at unit 0 with the aux_target_ result.
   * Float pins and u_resolution are still bound from the node's input pins.
   */
  void draw_pass(RenderTarget & target,
                 ShaderProgram &prog,
                 const GLuint   tex_id_override = 0) const {
    target.clear();
    prog.use();

    if (tex_id_override != 0) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex_id_override);
      prog.set_uniform("u_texture_0", 0);
    } else {
      bind_texture_inputs(&prog);
    }

    bind_float_inputs(&prog);
    prog.set_uniform("u_resolution",
                     target.get_fwidth(), target.get_fheight());

    ShaderQuadHelper::instance().render();

    ShaderProgram::unuse();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    RenderTarget::unbind();
  }

  // Binds Texture* input pins to texture units u_texture_0..N.
  void bind_texture_inputs(ShaderProgram *prog) const {
    int unit = 0;
    for (const auto &pin: inputs) {
      if (pin.data_type != DataType::Texture) continue;

      GLuint tex_id = 0;
      if (Texture *const *tex = pin.get_texture()) {
        if (*tex && (*tex)->is_valid()) tex_id = (*tex)->texture_id;
      }
      glActiveTexture(GL_TEXTURE0 + unit);
      glBindTexture(GL_TEXTURE_2D, tex_id);
      prog->set_uniform("u_texture_" + std::to_string(unit), unit);
      ++unit;
    }
  }

  // Sets float uniform "u_<pin.name>" for every float input pin.
  void bind_float_inputs(ShaderProgram *prog) const {
    for (const auto &pin: inputs) {
      if (pin.data_type != DataType::Float) continue;

      float value;
      if (const float *v = pin.get_float()) {
        value = *v;
      } else {
        // If the pin maps to a parameter modifiable via GUI, use the param value instead.
        value = get_param(pin.name);
      }

      prog->set_uniform("u_" + pin.name, value);
    }
  }

  // Unbinds all texture units used by Texture* input pins.
  void unbind_texture_inputs() const {
    int unit = 0;

    for (const auto &pin: inputs) {
      if (pin.data_type != DataType::Texture) continue;
      glActiveTexture(GL_TEXTURE0 + unit);
      glBindTexture(GL_TEXTURE_2D, 0);
      ++unit;
    }

    glActiveTexture(GL_TEXTURE0);
  }
};

#endif  // NAG_ENGINE_SHADER_NODE_H
