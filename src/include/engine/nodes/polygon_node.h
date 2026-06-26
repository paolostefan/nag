#ifndef NAG_ENGINE_POLYGON_NODE_H
#define NAG_ENGINE_POLYGON_NODE_H

#include "engine/property_widget.h"
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

  PolygonNode() {
    type = NodeType::Polygon;
    name = "Polygon";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Polygon"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "polygon"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kpolygon_frag; }

  void bind_params() override {
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_radius", radius);
    shader->set_uniform("u_rotation", rotation);
    shader->set_uniform("u_n_sides", static_cast<float>(n_sides));
    shader->set_uniform("u_color", color.x, color.y, color.z, color.w);
    shader->set_uniform("u_edge_smoothness", edge_smoothness);
  }

  // ── get_param — bridge for bind_float_inputs() ────────────────────────────

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "pos_x") return position.x;
    if (param_name == "pos_y") return position.y;
    if (param_name == "radius") return radius;
    if (param_name == "rotation") return rotation;
    return 0.f;
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j     = VisualNode::serialize_params();
    j["radius"]          = radius;
    j["rotation"]        = rotation;
    j["n_sides"]         = n_sides;
    j["color"]           = {color.x, color.y, color.z, color.w};
    j["edge_smoothness"] = edge_smoothness;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (auto result = VisualNode::deserialize_params(j); !result) return result;

      if (j.contains("radius")) radius = j["radius"];
      if (j.contains("rotation")) rotation = j["rotation"];
      if (j.contains("n_sides")) n_sides = j["n_sides"];
      if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 4) {
        color.x = j["color"][0];
        color.y = j["color"][1];
        color.z = j["color"][2];
        color.w = j["color"][3];
      }
      if (j.contains("edge_smoothness")) edge_smoothness = j["edge_smoothness"];

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize PolygonNode params: ") + e.what());
    }
  }

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    auto *color_ = reinterpret_cast<ImVec4 *>(&color);
    PropertyWidget::ColorEdit4("Color", id,
                               *color_,
                               [](Node &n, const ImVec4 &v) { dynamic_cast<PolygonNode &>(n).color = v; },
                               graph, history);

    // n_sides: int slider, intentionally not a pin (see class note).
    // Wrapped manually in undo/redo via SetNodeParamCommand if needed.
    ImGui::SliderInt("Sides", &n_sides, 3, 12);

    PropertyWidget::SliderFloat("Edge smoothness", id,
                                edge_smoothness,
                                [](Node &n, const float v) { dynamic_cast<PolygonNode &>(n).edge_smoothness = v; },
                                graph, history,
                                /*min=*/0.f, /*max=*/1.f,
                                /*format=*/"%.3f");
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
