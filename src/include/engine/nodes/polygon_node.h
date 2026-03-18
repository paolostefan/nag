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
 * Inputs (float streams):
 *   pos_x, pos_y  — centre in normalised [0,1] UV space
 *   radius         — circumradius in UV space
 *   rotation       — rotation in radians
 *
 * Output: Texture*
 *
 * @note n_sides is intentionally not exposed as a pin — fractional side
 *       counts produce undefined SDF geometry. It is a GUI-only parameter.
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

  [[nodiscard]] const char *shader_name()     const override { return "polygon"; }
  [[nodiscard]] const char *frag_shader_src() const override { return kpolygon_frag; }

  void bind_params() override {
    shader->set_uniform("u_position",        position.x, position.y);
    shader->set_uniform("u_radius",          radius);
    shader->set_uniform("u_rotation",        rotation);
    shader->set_uniform("u_n_sides",         static_cast<float>(n_sides));
    shader->set_uniform("u_color",           color.x, color.y, color.z, color.w);
    shader->set_uniform("u_edge_smoothness", edge_smoothness);
  }

  void render() override {
    update_from_inputs();
    ShaderNode::render();
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["position"]        = {position.x, position.y};
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

      if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 2) {
        position.x = j["position"][0];
        position.y = j["position"][1];
      }
      if (j.contains("radius"))          radius          = j["radius"];
      if (j.contains("rotation"))        rotation        = j["rotation"];
      if (j.contains("n_sides"))         n_sides         = j["n_sides"];
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

    // n_sides: int slider, not exposed as pin (see class note)
    int sides = n_sides;
    if (ImGui::SliderInt("Sides", &sides, 3, 12)) {
      n_sides = sides;
    }

    PropertyWidget::SliderFloat("Edge smoothness", id,
      edge_smoothness,
      [](Node &n, const float v) { dynamic_cast<PolygonNode &>(n).edge_smoothness = v; },
      graph, history, 0.f, 1.f);
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<PolygonNode> create(
      const Vec2  &position        = {0.5f, 0.5f},
      const float  radius          = 0.2f,
      const int    n_sides         = 6,
      const Vec4  &color           = Vec4::white(),
      const float  edge_smoothness = 0.01f)
  {
    auto node             = std::make_unique<PolygonNode>();
    node->position        = position;
    node->radius          = radius;
    node->n_sides         = n_sides;
    node->color           = color;
    node->edge_smoothness = edge_smoothness;

    node->add_input("pos_x");
    node->add_input("pos_y");
    node->add_input("radius");
    node->add_input("rotation");

    node->add_typed_output<Texture *>("texture");
    return node;
  }

private:
  void update_from_inputs() override {
    if (inputs.size() < 4) return;

    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[0].stream.get())) position.x = s->value;
    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[1].stream.get())) position.y = s->value;
    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[2].stream.get())) radius     = s->value;
    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[3].stream.get())) rotation   = s->value;
  }
};

#endif  // NAG_ENGINE_POLYGON_NODE_H
