#ifndef NAG_ENGINE_TRANSFORM_NODE_H
#define NAG_ENGINE_TRANSFORM_NODE_H

#include "engine/nodes/shader_node.h"
#include "shaders/transform_frag.h"

/**
 * @class TransformNode
 * @brief Applies 2D transform (translate, scale, rotate) to an input texture.
 *
 * The transform is computed in UV space around the center (0.5, 0.5).
 * Fragments mapping outside [0,1] after the inverse transform are transparent.
 *
 * Inputs:
 *   texture      (Texture*) — source image
 *   translate_x  (float)    — horizontal offset in normalized UV space
 *   translate_y  (float)    — vertical offset in normalized UV space
 *   scale        (float)    — uniform scale factor (1.0 = original size)
 *   rotation     (float)    — rotation in radians
 *
 * Output: Texture*
 */
struct TransformNode : ShaderNode {
  float translate_x{0.f};
  float translate_y{0.f};
  float scale{1.f};
  float rotation{0.f};
  std::vector<Property> props_;

  TransformNode() {
    type = NodeType::Transform;
    name = "Transform";
    props_ = {
      MakeFloatProp(*this, &TransformNode::translate_x, "translate_x", "Translate X",
                    WidgetKind::DragFloat, -1.f, 1.f, "%.3f", SliderFlag::None),
      MakeFloatProp(*this, &TransformNode::translate_y, "translate_y", "Translate Y",
                    WidgetKind::DragFloat, -1.f, 1.f, "%.3f", SliderFlag::None),
      MakeFloatProp(*this, &TransformNode::scale, "scale", "Scale",
                    WidgetKind::DragFloat, 0.01f, 10.f, "%.3f", SliderFlag::None),
      MakeFloatProp(*this, &TransformNode::rotation, "rotation", "Rotation",
                    WidgetKind::DragFloat, -3.14159265f, 3.14159265f, "%.3f", SliderFlag::None),
    };
    props_[0].disable_pin = "translate_x";
    props_[1].disable_pin = "translate_y";
    props_[2].disable_pin = "scale";
    props_[3].disable_pin = "rotation";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Transform"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "transform"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return ktransform_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void bind_params() override {
    shader->set_uniform("u_translate_x", translate_x);
    shader->set_uniform("u_translate_y", translate_y);
    shader->set_uniform("u_scale", scale);
    shader->set_uniform("u_rotation", rotation);
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<TransformNode> create(
    const float translate_x = 0.f,
    const float translate_y = 0.f,
    const float scale       = 1.f,
    const float rotation    = 0.f) {
    auto node         = std::make_unique<TransformNode>();
    node->translate_x = translate_x;
    node->translate_y = translate_y;
    node->scale       = scale;
    node->rotation    = rotation;

    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "translate_x");
    node->add_input(DataType::Float, "translate_y");
    node->add_input(DataType::Float, "scale");
    node->add_input(DataType::Float, "rotation");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

  void update_from_inputs() override {
    // inputs[0] is Texture* — handled by bind_texture_inputs() in ShaderNode::render()
    read_pin_to("translate_x", translate_x);
    read_pin_to("translate_y", translate_y);
    read_pin_to("scale", scale);
    read_pin_to("rotation", rotation);
  }
};

#endif  // NAG_ENGINE_TRANSFORM_NODE_H
