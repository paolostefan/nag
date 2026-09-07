#ifndef NAG_ENGINE_COLOR_CORRECTION_NODE_H
#define NAG_ENGINE_COLOR_CORRECTION_NODE_H

#include "engine/nodes/shader_node.h"
#include "shaders/color_correction_frag.h"

/**
 * @class ColorCorrectionNode
 * @brief Applies brightness, contrast, saturation and hue shift to a texture.
 *
 * Inputs:
 *   texture     (Texture*) — source image
 *   brightness  (float)    — additive offset    [-1, 1],  0 = no change
 *   contrast    (float)    — multiplier          [ 0, 4],  1 = no change
 *   saturation  (float)    — 0 = greyscale,      1 = original, >1 = boost
 *   hue_shift   (float)    — hue rotation in radians [0, 2π]
 *
 * Output: Texture*
 */
struct ColorCorrectionNode : ShaderNode {
  float brightness{0.f};
  float contrast{1.f};
  float saturation{1.f};
  float hue_shift{0.f};
  std::vector<Property> props_;

  ColorCorrectionNode() {
    type = NodeType::ColorCorrection;
    name = "Color Correction";
    props_ = {
      MakeFloatProp(*this, &ColorCorrectionNode::brightness, "brightness", "Brightness",
                    WidgetKind::SliderFloat, -1.f, 1.f, "%.2f"),
      MakeFloatProp(*this, &ColorCorrectionNode::contrast, "contrast", "Contrast",
                    WidgetKind::SliderFloat, 0.f, 4.f, "%.2f"),
      MakeFloatProp(*this, &ColorCorrectionNode::saturation, "saturation", "Saturation",
                    WidgetKind::SliderFloat, 0.f, 2.f, "%.2f"),
      MakeFloatProp(*this, &ColorCorrectionNode::hue_shift, "hue_shift", "Hue Shift",
                    WidgetKind::SliderFloat, 0.f, 6.28318f, "%.2f"),
    };
    props_[0].disable_pin = "brightness";
    props_[1].disable_pin = "contrast";
    props_[2].disable_pin = "saturation";
    props_[3].disable_pin = "hue_shift";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Color Correction"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "color_correction"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kcolor_correction_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void bind_params() override {
    shader->set_uniform("u_brightness", brightness);
    shader->set_uniform("u_contrast", contrast);
    shader->set_uniform("u_saturation", saturation);
    shader->set_uniform("u_hue_shift", hue_shift);
  }

  void update_from_inputs() override {
    read_pin_to("brightness", brightness);
    read_pin_to("contrast", contrast);
    read_pin_to("saturation", saturation);
    read_pin_to("hue_shift", hue_shift);
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<ColorCorrectionNode> create(
    const float brightness = 0.f,
    const float contrast = 1.f,
    const float saturation = 1.f,
    const float hue_shift = 0.f) {
    auto node = std::make_unique<ColorCorrectionNode>();
    node->brightness = brightness;
    node->contrast = contrast;
    node->saturation = saturation;
    node->hue_shift = hue_shift;

    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "brightness");
    node->add_input(DataType::Float, "contrast");
    node->add_input(DataType::Float, "saturation");
    node->add_input(DataType::Float, "hue_shift");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

};

#endif  // NAG_ENGINE_COLOR_CORRECTION_NODE_H
