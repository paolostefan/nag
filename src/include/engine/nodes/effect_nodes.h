#ifndef NAG_ENGINE_EFFECT_NODES_H
#define NAG_ENGINE_EFFECT_NODES_H

#include "engine/nodes/shader_node.h"
#include "shaders/blur_frag.h"
#include "shaders/chromatic_aberration_frag.h"
#include "shaders/pixelate_frag.h"

// ============================================================================
// BlurNode — Gaussian blur, separable two-pass
// ============================================================================
//
// Pass 1 (horizontal): reads from input Texture*, writes to aux_target_.
// Pass 2 (vertical):   reads from aux_target_, writes to render_target.
//
// Pins:
//   Input  0: Texture*  "texture"  — source image
//   Input  1: float     "radius"   — blur radius in pixels [0..32], default 4.0
//   Output 0: Texture*  "texture"  — blurred result
//
// Shader uniforms expected by shaders/blur.frag:
//   uniform sampler2D u_texture_0;
//   uniform float     u_radius;
//   uniform vec2      u_resolution;
//   uniform int       u_horizontal;   // 1 = horizontal pass, 0 = vertical
//
struct BlurNode : ShaderNode {
  float radius{4.f};

  BlurNode() {
    type = NodeType::Blur;
    name = "Blur";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Blur"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "blur_gaussian"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kblur_frag; }

  bool initialize(const int width, const int height) override {
    if (!ShaderNode::initialize(width, height)) return false;
    return initialize_aux(width, height);
  }

  void render() override {
    if (!render_target || !render_target->is_valid() ||
        !aux_target_ || !aux_target_->is_valid() ||
        !shader || !shader->is_valid())
      return;

    // ── Pass 1: horizontal ────────────────────────────────────────────────
    shader->use();
    shader->set_uniform("u_horizontal", 1);
    ShaderProgram::unuse();

    draw_pass(*aux_target_, *shader); // reads from input pin Texture*

    // ── Pass 2: vertical ──────────────────────────────────────────────────
    shader->use();
    shader->set_uniform("u_horizontal", 0);
    ShaderProgram::unuse();

    draw_pass(*render_target, *shader, // reads from aux_target_ result
              aux_target_->get_texture());
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["radius"] = radius;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    if (auto result = VisualNode::deserialize_params(j); !result) {
      return result;
    }
    // Re-init aux after base re-initialized the main RenderTarget.
    if (render_target) {
      initialize_aux(render_target->get_width(), render_target->get_height());
    }
    if (j.contains("radius")) radius = j["radius"];
    return OperationResult::ok();
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "radius") return radius;
    return 0.f;
  }

  void update_from_inputs() override {
    // If radius pin is connected, override the member variable with the input value.
    if (const Pin *radius_pin = get_input("radius");
      radius_pin && radius_pin->connected) {
      if (const float *s = radius_pin->get_float()) {
        radius = *s;
      }
    }
  }

  [[nodiscard]] static std::unique_ptr<BlurNode> create(const float radius = 4.f) {
    auto node = std::make_unique<BlurNode>();
    node->radius = radius;
    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "radius"); // animatable via pin
    node->add_output(DataType::Texture, "texture");
    return node;
  }
};

// ============================================================================
// ChromaticAberrationNode — RGB channel offset
// ============================================================================
//
// Offsets the R and B channels by ±strength pixels along the X axis,
// leaving G unchanged. Classic lens fringing effect.
//
// Pins:
//   Input  0: Texture*  "texture"   — source image
//   Input  1: float     "strength"  — pixel offset per channel [0..20], default 3.0
//   Output 0: Texture*  "texture"   — result
//
// Shader uniforms expected by shaders/chromatic_aberration.frag:
//   uniform sampler2D u_texture_0;
//   uniform float     u_strength;
//   uniform vec2      u_resolution;
//
struct ChromaticAberrationNode : ShaderNode {
  float strength{3.f};

  ChromaticAberrationNode() {
    type = NodeType::ChromaticAberration;
    name = "ChromaticAberration";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Chromatic Aberration"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "chromatic_aberration"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kchromatic_aberration_frag; }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["strength"] = strength;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    if (auto result = VisualNode::deserialize_params(j); !result) {
      return result;
    }

    if (j.contains("strength")) strength = j["strength"];
    return OperationResult::ok();
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "strength") return strength;
    return 0.f;
  }

  [[nodiscard]] static std::unique_ptr<ChromaticAberrationNode> create(
    const float strength = 3.f) {
    auto node = std::make_unique<ChromaticAberrationNode>();
    node->strength = strength;
    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "strength");
    node->add_output(DataType::Texture, "texture");
    return node;
  }
};

// ============================================================================
// PixelateNode — pixelation / mosaic effect
// ============================================================================
//
// Quantises UVs to blocks of pixel_size x pixel_size pixels.
//
// Pins:
//   Input  0: Texture*  "texture"     — source image
//   Input  1: float     "pixel_size"  — block size in pixels [1..64], default 8.0
//   Output 0: Texture*  "texture"     — pixelated result
//
// Shader uniforms expected by shaders/pixelate.frag:
//   uniform sampler2D u_texture_0;
//   uniform float     u_pixel_size;
//   uniform vec2      u_resolution;
//
struct PixelateNode : ShaderNode {
  float pixel_size{8.f};

  PixelateNode() {
    type = NodeType::Pixelate;
    name = "Pixelate";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Pixelate"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "pixelate"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kpixelate_frag; }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["pixel_size"] = pixel_size;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    if (auto result = VisualNode::deserialize_params(j); !result) {
      return result;
    }

    if (j.contains("pixel_size")) pixel_size = j["pixel_size"];
    return OperationResult::ok();
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "pixel_size") return pixel_size;
    return 0.f;
  }

  [[nodiscard]] static std::unique_ptr<PixelateNode> create(
    const float pixel_size = 8.f) {
    auto node = std::make_unique<PixelateNode>();
    node->pixel_size = pixel_size;
    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "pixel_size");
    node->add_output(DataType::Texture, "texture");
    return node;
  }
};

#endif  // NAG_ENGINE_EFFECT_NODES_H
