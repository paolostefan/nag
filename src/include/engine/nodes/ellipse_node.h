#ifndef NAG_ENGINE_ELLIPSE_NODE_H
#define NAG_ENGINE_ELLIPSE_NODE_H

#include "engine/property_widget.h"
#include "engine/nodes/shader_node.h"
#include "engine/visual_types.h"
#include "shaders/ellipse_frag.h"

/**
 * @class EllipseNode
 * @brief Renders an ellipse (or circle) to texture via SDF fragment shader.
 *
 * Inputs (float streams):
 *   pos_x, pos_y   — centre in normalised [0,1] UV space
 *   radius_x        — horizontal radius in UV space
 *   radius_y        — vertical  radius in UV space
 *   rotation        — rotation in radians
 *
 * Output: Texture*
 */
struct EllipseNode : ShaderNode {
  Vec2  position{0.5f, 0.5f};
  float radius_x{0.3f};
  float radius_y{0.15f};
  float rotation{0.f};
  Vec4  color{1.f, 1.f, 1.f, 1.f};
  float edge_smoothness{0.01f};

  EllipseNode() {
    type = NodeType::Ellipse;
    name = "Ellipse";
  }

  [[nodiscard]] const char *shader_name()    const override { return "ellipse"; }
  [[nodiscard]] const char *frag_shader_src() const override { return kellipse_frag; }

  void bind_params() override {
    shader->set_uniform("u_position",        position.x, position.y);
    shader->set_uniform("u_radius_x",        radius_x);
    shader->set_uniform("u_radius_y",        radius_y);
    shader->set_uniform("u_rotation",        rotation);
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
    j["radius_x"]        = radius_x;
    j["radius_y"]        = radius_y;
    j["rotation"]        = rotation;
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
      if (j.contains("radius_x"))        radius_x        = j["radius_x"];
      if (j.contains("radius_y"))        radius_y        = j["radius_y"];
      if (j.contains("rotation"))        rotation        = j["rotation"];
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
        std::string("Failed to deserialize EllipseNode params: ") + e.what());
    }
  }

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    auto *color_ = reinterpret_cast<ImVec4 *>(&color);
    PropertyWidget::ColorEdit4("Color", id,
      *color_,
      [](Node &n, const ImVec4 &v) { dynamic_cast<EllipseNode &>(n).color = v; },
      graph, history);

    PropertyWidget::SliderFloat("Edge smoothness", id,
      edge_smoothness,
      [](Node &n, const float v) { dynamic_cast<EllipseNode &>(n).edge_smoothness = v; },
      graph, history, 0.f, 1.f);
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<EllipseNode> create(
      const Vec2  &position       = {0.5f, 0.5f},
      const float  radius_x       = 0.3f,
      const float  radius_y       = 0.15f,
      const Vec4  &color          = Vec4::white(),
      const float  edge_smoothness = 0.01f)
  {
    auto node              = std::make_unique<EllipseNode>();
    node->position         = position;
    node->radius_x         = radius_x;
    node->radius_y         = radius_y;
    node->color            = color;
    node->edge_smoothness  = edge_smoothness;

    node->add_input("pos_x");
    node->add_input("pos_y");
    node->add_input("radius_x");
    node->add_input("radius_y");
    node->add_input("rotation");

    node->add_typed_output<Texture *>("texture");
    return node;
  }

private:
  void update_from_inputs() override {
    if (inputs.size() < 5) return;

    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[0].stream.get())) position.x = s->value;
    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[1].stream.get())) position.y = s->value;
    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[2].stream.get())) radius_x   = s->value;
    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[3].stream.get())) radius_y   = s->value;
    if (const auto *s = dynamic_cast<Stream<float> *>(inputs[4].stream.get())) rotation   = s->value;
  }
};

#endif  // NAG_ENGINE_ELLIPSE_NODE_H
