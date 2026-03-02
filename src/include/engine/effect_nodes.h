#ifndef NAG_ENGINE_EFFECT_NODES_H
#define NAG_ENGINE_EFFECT_NODES_H

#include "engine/shader_node.h"

#include "editor/property_widget.h"

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
  float radius{4.0f};

  BlurNode() {
    type = NodeType::Blur;
    name = "Blur";
  }

  [[nodiscard]] const char *shader_name() const override { return "blur_gaussian"; }
  [[nodiscard]] const char *frag_shader_path() const override { return "shaders/blur.frag"; }

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

  void bind_params() override {
    // radius is already bound as u_radius via bind_float_inputs().
    // Nothing extra needed here.
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["radius"] = radius;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    auto result = VisualNode::deserialize_params(j);
    if (!result) return result;
    // Re-init aux after base re-initialised the main RenderTarget.
    if (render_target) {
      initialize_aux(render_target->get_width(), render_target->get_height());
    }
    if (j.contains("radius")) radius = j["radius"];
    return OperationResult::ok();
  }

  [[nodiscard]] static std::unique_ptr<BlurNode> create(const float radius = 4.0f) {
    auto node = std::make_unique<BlurNode>();
    node->radius = radius;
    node->add_typed_input<Texture *>("texture");
    node->add_typed_input<float>("radius"); // animatable via pin
    node->add_typed_output<Texture *>("texture");
    return node;
  }

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    // ------------------------------------------------------------------
    // Radius — slider con range [0, 64]
    // ------------------------------------------------------------------
    PropertyWidget::SliderFloat(
      "Radius",
      /*node_id=*/id,
      /*value=*/radius,
      /*setter=*/[](Node &n, float v) {
        dynamic_cast<BlurNode &>(n).radius = v;
      },
      graph, history,
      /*min=*/0.0f, /*max=*/64.0f);

    // ------------------------------------------------------------------
    // Sigma — drag (nessun limite fisso, ma tipicamente [0.1, 20])
    // // ------------------------------------------------------------------
    // PropertyWidget::DragFloat(
    //   "Sigma",
    //   /*node_id=*/id,
    //   /*value=*/sigma,
    //   /*setter=*/[](Node &n, float v) {
    //     dynamic_cast<BlurNode &>(n).sigma = v;
    //   },
    //   graph, history,
    //   /*speed=*/0.05f, /*min=*/0.01f, /*max=*/20.0f);
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
  float strength{3.0f};

  ChromaticAberrationNode() {
    type = NodeType::ChromaticAberration;
    name = "ChromaticAberration";
  }

  [[nodiscard]] const char *shader_name() const override { return "chromatic_aberration"; }
  [[nodiscard]] const char *frag_shader_path() const override { return "shaders/chromatic_aberration.frag"; }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["strength"] = strength;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    auto result = VisualNode::deserialize_params(j);
    if (!result) return result;
    if (j.contains("strength")) strength = j["strength"];
    return OperationResult::ok();
  }

  [[nodiscard]] static std::unique_ptr<ChromaticAberrationNode> create(
    const float strength = 3.0f) {
    auto node = std::make_unique<ChromaticAberrationNode>();
    node->strength = strength;
    node->add_typed_input<Texture *>("texture");
    node->add_typed_input<float>("strength");
    node->add_typed_output<Texture *>("texture");
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
  float pixel_size{8.0f};

  PixelateNode() {
    type = NodeType::Pixelate;
    name = "Pixelate";
  }

  [[nodiscard]] const char *shader_name() const override { return "pixelate"; }
  [[nodiscard]] const char *frag_shader_path() const override { return "shaders/pixelate.frag"; }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["pixel_size"] = pixel_size;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    auto result = VisualNode::deserialize_params(j);
    if (!result) return result;
    if (j.contains("pixel_size")) pixel_size = j["pixel_size"];
    return OperationResult::ok();
  }

  [[nodiscard]] static std::unique_ptr<PixelateNode> create(
    const float pixel_size = 8.0f) {
    auto node = std::make_unique<PixelateNode>();
    node->pixel_size = pixel_size;
    node->add_typed_input<Texture *>("texture");
    node->add_typed_input<float>("pixel_size");
    node->add_typed_output<Texture *>("texture");
    return node;
  }
};

#endif  // NAG_ENGINE_EFFECT_NODES_H
