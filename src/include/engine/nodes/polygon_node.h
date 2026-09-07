#ifndef NAG_ENGINE_POLYGON_NODE_H
#define NAG_ENGINE_POLYGON_NODE_H

#include "engine/nodes/shader_node.h"
#include "engine/visual_types.h"
#include "shaders/polygon_frag.h"

/**
 * @class PolygonNode
 * @brief Renders a regular n-gon to texture via SDF fragment shader.
 *
 * Inputs:
 *   pos_x     (float)  — centre X in normalised [0,1] UV space
 *   pos_y     (float)  — centre Y in normalised [0,1] UV space
 *   radius    (float)  — circumradius (fraction of screen height)
 *   rotation  (float)  — rotation in radians
 *
 * Output: Texture*
 *
 * @note n_sides is GUI-only — fractional values produce undefined SDF geometry.
 */
struct PolygonNode : ShaderNode {
  Vec2  position{0.5f, 0.5f};
  float radius{0.2f};
  float rotation{0.f};
  int   n_sides{6};
  Vec4  color{1.f, 1.f, 1.f, 1.f};
  float edge_smoothness{0.01f};
  std::vector<Property> props_;

  PolygonNode() {
    type = NodeType::Polygon;
    name = "Polygon";
    props_ = {
      MakeColorProp(*this, &PolygonNode::color, "color", "Color"),
      MakeIntProp(*this, &PolygonNode::n_sides, "n_sides", "Sides",
                  WidgetKind::SliderInt, 3.f, 12.f, "%d"),
      MakeFloatProp(*this, &PolygonNode::edge_smoothness, "edge_smoothness", "Edge Smoothness",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.3f"),
      MakeFloatProp(*this, &PolygonNode::radius, "radius", "Radius",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.3f"),
      MakeFloatProp(*this, &PolygonNode::rotation, "rotation", "Rotation",
                    WidgetKind::SliderFloat, -3.14159265f, 3.14159265f, "%.3f"),
    };
    props_[3].disable_pin = "radius";
    props_[4].disable_pin = "rotation";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Polygon"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "polygon"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kpolygon_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void bind_params() override {
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_radius", radius);
    shader->set_uniform("u_rotation", rotation);
    shader->set_uniform("u_n_sides", static_cast<float>(n_sides));
    shader->set_uniform("u_color", color.x, color.y, color.z, color.w);
    shader->set_uniform("u_edge_smoothness", edge_smoothness);
  }

  // ── get_param — bridge for bind_float_inputs() ────────────────────────────
  // pos_x/pos_y live in the composite `position` Vec2.

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "pos_x") return position.x;
    if (param_name == "pos_y") return position.y;
    return Node::get_param(param_name);
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j     = VisualNode::serialize_params();
    j["color"]           = {color.x, color.y, color.z, color.w};
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
        std::string("Failed to deserialize PolygonNode params: ") + e.what());
    }
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<PolygonNode> create(
    const Vec2 &position        = {0.5f, 0.5f},
    const float radius          = 0.2f,
    const int   n_sides         = 6,
    const Vec4 &color           = Vec4::white(),
    const float edge_smoothness = 0.01f) {
    auto node             = std::make_unique<PolygonNode>();
    node->position        = position;
    node->radius          = radius;
    node->n_sides         = n_sides;
    node->color           = color;
    node->edge_smoothness = edge_smoothness;

    node->add_input(DataType::Float, "pos_x");
    node->add_input(DataType::Float, "pos_y");
    node->add_input(DataType::Float, "radius");
    node->add_input(DataType::Float, "rotation");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

  void update_from_inputs() override {
    read_pin_to("pos_x", position.x);
    read_pin_to("pos_y", position.y);
    read_pin_to("radius", radius);
    read_pin_to("rotation", rotation);
  }
};

#endif  // NAG_ENGINE_POLYGON_NODE_H
