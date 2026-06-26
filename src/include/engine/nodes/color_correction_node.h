#ifndef NAG_ENGINE_COLOR_CORRECTION_NODE_H
#define NAG_ENGINE_COLOR_CORRECTION_NODE_H

#include "engine/nodes/shader_node.h"
#include "engine/property_widget.h"
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

  ColorCorrectionNode() {
    type = NodeType::ColorCorrection;
    name = "Color Correction";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Color Correction"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "color_correction"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kcolor_correction_frag; }

  void bind_params() override {
    shader->set_uniform("u_brightness", brightness);
    shader->set_uniform("u_contrast", contrast);
    shader->set_uniform("u_saturation", saturation);
    shader->set_uniform("u_hue_shift", hue_shift);
  }

  // ── get_param — bridge for bind_float_inputs() ────────────────────────────

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "brightness") return brightness;
    if (param_name == "contrast") return contrast;
    if (param_name == "saturation") return saturation;
    if (param_name == "hue_shift") return hue_shift;
    return 0.f;
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["brightness"] = brightness;
    j["contrast"] = contrast;
    j["saturation"] = saturation;
    j["hue_shift"] = hue_shift;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (auto result = VisualNode::deserialize_params(j); !result) return result;

      if (j.contains("brightness")) brightness = j["brightness"];
      if (j.contains("contrast")) contrast = j["contrast"];
      if (j.contains("saturation")) saturation = j["saturation"];
      if (j.contains("hue_shift")) hue_shift = j["hue_shift"];

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize ColorCorrectionNode params: ") + e.what());
    }
  }

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    PropertyWidget::SliderFloat("Brightness", id,
                                brightness,
                                [](Node &n, const float v) { dynamic_cast<ColorCorrectionNode &>(n).brightness = v; },
                                graph, history,
                                /*min=*/-1.f, /*max=*/1.f, /*format=*/"%.2f",
                                /*disabled=*/get_input("brightness")->connected);

    PropertyWidget::SliderFloat("Contrast", id,
                                contrast,
                                [](Node &n, const float v) { dynamic_cast<ColorCorrectionNode &>(n).contrast = v; },
                                graph, history,
                                /*min=*/0.f, /*max=*/4.f, /*format=*/"%.2f",
                                /*disabled=*/get_input("contrast")->connected);

    PropertyWidget::SliderFloat("Saturation", id,
                                saturation,
                                [](Node &n, const float v) { dynamic_cast<ColorCorrectionNode &>(n).saturation = v; },
                                graph, history,
                                /*min=*/0.f, /*max=*/2.f, /*format=*/"%.2f",
                                /*disabled=*/get_input("saturation")->connected);

    PropertyWidget::SliderFloat("Hue shift", id,
                                hue_shift,
                                [](Node &n, const float v) { dynamic_cast<ColorCorrectionNode &>(n).hue_shift = v; },
                                graph, history,
                                /*min=*/0.f, /*max=*/6.2832f, /*format=*/"%.2f",
                                /*disabled=*/get_input("hue_shift")->connected);
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
