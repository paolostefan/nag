#ifndef NAG_ENGINE_ELLIPSE_NODE_H
#define NAG_ENGINE_ELLIPSE_NODE_H

#include "engine/nodes/shader_node.h"
#include "engine/visual_types.h"
#include "shaders/ellipse_frag.h"

/**
 * @class EllipseNode
 * @brief Renders an ellipse to texture via SDF fragment shader.
 *
 * Inputs:
 *   pos_x     (float)  — center X in normalized [0,1] UV space
 *   pos_y     (float)  — center Y in normalized [0,1] UV space
 *   radius_x  (float)  — horizontal radius (fraction of screen height)
 *   radius_y  (float)  — vertical radius   (fraction of screen height)
 *   rotation  (float)  — rotation in radians
 *
 * Output: Texture*
 */
struct EllipseNode : ShaderNode {
  Vec2 position{.5f, .5f};
  float radius_x{.3f};
  float radius_y{.15f};
  float rotation{0.f};
  Vec4 color{1.f, 1.f, 1.f, 1.f};
  float edge_smoothness{.01f};
  std::vector<Property> props_;

  EllipseNode() {
    type = NodeType::Ellipse;
    name = "Ellipse";
    props_ = {
      MakeColorProp(*this, &EllipseNode::color, "color", "Color"),
      MakeFloatProp(*this, &EllipseNode::radius_x, "radius_x", "Radius X",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.3f"),
      MakeFloatProp(*this, &EllipseNode::radius_y, "radius_y", "Radius Y",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.3f"),
      MakeFloatProp(*this, &EllipseNode::rotation, "rotation", "Rotation",
                    WidgetKind::SliderFloat, -3.14159265f, 3.14159265f, "%.3f"),
      MakeFloatProp(*this, &EllipseNode::edge_smoothness, "edge_smoothness", "Edge Smoothness",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.3f"),
    };
    props_[1].disable_pin = "radius_x";
    props_[2].disable_pin = "radius_y";
    props_[3].disable_pin = "rotation";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Ellipse"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "ellipse"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kellipse_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void bind_params() override {
    shader->set_uniform("u_position", position);
    shader->set_uniform("u_radius_x", radius_x);
    shader->set_uniform("u_radius_y", radius_y);
    shader->set_uniform("u_rotation", rotation);
    shader->set_uniform("u_color", color);
    shader->set_uniform("u_edge_smoothness", edge_smoothness);
  }

  // ── get_param — bridge for bind_float_inputs() ────────────────────────────
  // pos_x/pos_y live in the composite `position` Vec2 (not serialized); schema
  // covers radius_x/radius_y/rotation.

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "pos_x") return position.x;
    if (param_name == "pos_y") return position.y;
    return Node::get_param(param_name);
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();

    j["color"] = {color.x, color.y, color.z, color.w};
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (auto result = VisualNode::deserialize_params(j); !result) return result;

      if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 4) {
        color.x = j["color"][0];
        color.y = j["color"][1];
        color.z = j["color"][2];
        color.w = j["color"][3];
      }

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize EllipseNode params: ") + e.what());
    }
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<EllipseNode> create(
    const Vec2 &position = {.5f, .5f},
    const float radius_x = .3f,
    const float radius_y = .15f,
    const Vec4 &color = Vec4::white(),
    const float edge_smoothness = .01f) {
    auto node = std::make_unique<EllipseNode>();
    node->position = position;
    node->radius_x = radius_x;
    node->radius_y = radius_y;
    node->color = color;
    node->edge_smoothness = edge_smoothness;

    node->add_input(DataType::Float, "pos_x");
    node->add_input(DataType::Float, "pos_y");
    node->add_input(DataType::Float, "radius_x");
    node->add_input(DataType::Float, "radius_y");
    node->add_input(DataType::Float, "rotation");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

  void update_from_inputs() override {
    read_pin_to("pos_x", position.x);
    read_pin_to("pos_y", position.y);
    read_pin_to("radius_x", radius_x);
    read_pin_to("radius_y", radius_y);
    read_pin_to("rotation", rotation);
  }
};

#endif  // NAG_ENGINE_ELLIPSE_NODE_H
