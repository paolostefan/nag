#ifndef NAG_ENGINE_SDF_SHAPE_NODE_H
#define NAG_ENGINE_SDF_SHAPE_NODE_H

#include "engine/nodes/shader_node.h"
#include "engine/property_widget.h"
#include "engine/visual_types.h"
#include "shaders/sdf_shape_frag.h"

/**
 * @class SDFShapeNode
 * @brief Renders a circle, box, or ring via a unified SDF fragment shader.
 *
 * All shapes share the same pins; shape-specific parameters (aspect ratio for
 * box, ring thickness) are GUI-only because they have no meaningful analogue
 * across all three shapes.
 *
 * Inputs:
 *   pos_x    (float)  — centre X in normalised [0,1] UV space
 *   pos_y    (float)  — centre Y in normalised [0,1] UV space
 *   radius   (float)  — circumradius / half-extent (fraction of screen height)
 *   rotation (float)  — rotation in radians
 *
 * Output: Texture*
 */
struct SDFShapeNode : ShaderNode {
  enum class Shape:uint8_t { Circle = 0, Box = 1, Ring = 2 };

  Vec4  color{1.f, 1.f, 1.f, 1.f};
  Vec2  position{0.5f, 0.5f};
  float radius{0.2f};
  float rotation{0.f};
  float aspect{1.f};          // box only: width/height ratio
  float ring_thickness{0.2f}; // ring only: [0, 1]
  float edge_smoothness{0.01f};
  Shape shape{Shape::Circle};

  SDFShapeNode() {
    type = NodeType::SDFShape;
    name = "SDF Shape";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "SDF Shape"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "sdf_shape"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return ksdf_shape_frag; }

  void bind_params() override {
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_radius", radius);
    shader->set_uniform("u_rotation", rotation);
    shader->set_uniform("u_shape", static_cast<float>(shape));
    shader->set_uniform("u_aspect", aspect);
    shader->set_uniform("u_ring_thickness", ring_thickness);
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
    j["shape"]           = shape;
    j["aspect"]          = aspect;
    j["ring_thickness"]  = ring_thickness;
    j["color"]           = {color.x, color.y, color.z, color.w};
    j["edge_smoothness"] = edge_smoothness;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (auto result = VisualNode::deserialize_params(j); !result) return result;

      if (j.contains("radius")) radius = j["radius"];
      if (j.contains("rotation")) rotation = j["rotation"];
      if (j.contains("shape")) shape = static_cast<Shape>(j["shape"].get<int>());
      if (j.contains("aspect")) aspect = j["aspect"];
      if (j.contains("ring_thickness")) ring_thickness = j["ring_thickness"];
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
        std::string("Failed to deserialize SDFShapeNode params: ") + e.what());
    }
  }

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    // Shape selector
    static constexpr const char *kShapes[] = {"Circle", "Box", "Ring"};
    int                          shape_idx = static_cast<int>(shape);
    if (ImGui::Combo("Shape", &shape_idx, kShapes, 3)) {
      shape = static_cast<Shape>(shape_idx);
    }

    // Shape-specific parameters
    if (shape == Shape::Box) {
      ImGui::DragFloat("Aspect ratio", &aspect, 0.01f, 0.1f, 10.f, "%.2f");
    }
    if (shape == Shape::Ring) {
      ImGui::DragFloat("Ring thickness", &ring_thickness, 0.005f, 0.01f, 1.f, "%.3f");
    }

    auto *color_ = reinterpret_cast<ImVec4 *>(&color);
    PropertyWidget::ColorEdit4("Color", id,
                               *color_,
                               [](Node &n, const ImVec4 &v) { dynamic_cast<SDFShapeNode &>(n).color = v; },
                               graph, history);

    PropertyWidget::SliderFloat("Edge smoothness", id,
                                edge_smoothness,
                                [](Node &n, const float v) { dynamic_cast<SDFShapeNode &>(n).edge_smoothness = v; },
                                graph, history,
                                /*min=*/0.f, /*max=*/1.f, /*format=*/"%.3f");
  }

  void update_from_inputs() override {
    read_pin_to("pos_x", position.x);
    read_pin_to("pos_y", position.y);
    read_pin_to("radius", radius);
    read_pin_to("rotation", rotation);
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<SDFShapeNode> create(
    const Vec2 &position        = {0.5f, 0.5f},
    const float radius          = 0.2f,
    const Shape shape           = Shape::Circle,
    const Vec4 &color           = Vec4::white(),
    const float edge_smoothness = 0.01f) {
    auto node             = std::make_unique<SDFShapeNode>();
    node->position        = position;
    node->radius          = radius;
    node->shape           = shape;
    node->color           = color;
    node->edge_smoothness = edge_smoothness;

    node->add_input(DataType::Float, "pos_x");
    node->add_input(DataType::Float, "pos_y");
    node->add_input(DataType::Float, "radius");
    node->add_input(DataType::Float, "rotation");

    node->add_output(DataType::Texture, "texture");
    return node;
  }
};

#endif  // NAG_ENGINE_SDF_SHAPE_NODE_H
