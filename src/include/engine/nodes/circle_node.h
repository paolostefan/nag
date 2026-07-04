#ifndef NAG_ENGINE_CIRCLE_NODE_H
#define NAG_ENGINE_CIRCLE_NODE_H

#include "engine/property_widget.h"
#include "engine/nodes/shader_node.h"
#include "engine/visual_types.h"
#include "shaders/circle_frag.h"

/**
 * @class CircleNode
 * @brief Renders a circle to texture.
 */
struct CircleNode : ShaderNode {
  Vec4 color{1.f, 1.f, 1.f, 1.f};
  Vec2 position{0.5f, 0.5f};
  float radius{0.2f};
  float edge_smoothness{0.01f};

  CircleNode() {
    type = NodeType::Circle;
    name = "Circle";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Circle"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "circle"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kcircle_frag; }

  void bind_params() override {
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_radius", radius);
    shader->set_uniform("u_color", color.x, color.y, color.z, color.w);
    shader->set_uniform("u_edge_smoothness", edge_smoothness);
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["radius"] = radius;
    j["position"] = {position.x, position.y};
    j["color"] = {color.x, color.y, color.z, color.w};
    j["edge_smoothness"] = edge_smoothness;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      // Deserialize base class first
      if (auto result = VisualNode::deserialize_params(j); !result) {
        return result;
      }

      if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 4) {
        color.x = j["color"][0];
        color.y = j["color"][1];
        color.z = j["color"][2];
        color.w = j["color"][3];
      }

      if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 2) {
        position.x = j["position"][0];
        position.y = j["position"][1];
      }

      if (j.contains("radius")) {
        radius = j["radius"];
      }

      if (j.contains("edge_smoothness")) {
        edge_smoothness = j["edge_smoothness"];
      }

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize CircleNode params: ") + e.what()
      );
    }
  }

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    auto *color_ = reinterpret_cast<ImVec4 *>(&color);
    PropertyWidget::ColorEdit4("Color",
                               /* node_id=*/ id,
                               /* value=*/ *color_,
                               /* setter =*/[](Node &n, const ImVec4 &col) {
                                 dynamic_cast<CircleNode &>(n).color = col;
                               },
                               graph, history
    );

    // ------------------------------------------------------------------
    // Edge smoothness — slider with range [0, 1]
    // ------------------------------------------------------------------
    PropertyWidget::SliderFloat(
      "Edge smoothness",
      /*node_id=*/id,
      /*value=*/edge_smoothness,
      /*setter=*/[](Node &n, const float v) {
        dynamic_cast<CircleNode &>(n).edge_smoothness = v;
      },
      graph, history,
      /*min=*/0.f, /*max=*/1.f);
  }

  void update_from_inputs() override {
    if (inputs.size() >= 2) {
      if (const float *x = inputs[0].get_float()) {
        position.x = *x;
      }
      if (const float *y = inputs[1].get_float()) {
        position.y = *y;
      }
    }

    if (inputs.size() >= 3) {
      if (const float *r = inputs[2].get_float()) {
        radius = *r;
      }
    }
  }

public:
  /**
   * Create a circle node.
   */
  static std::unique_ptr<CircleNode> create(const Vec2 &position = {0.5f, 0.5f},
                                            const float radius = 0.2f,
                                            const Vec4 &color = Vec4::white()) {
    auto node = std::make_unique<CircleNode>();
    node->position = position;
    node->radius = radius;
    node->color = color;
    node->add_input(DataType::Float, "pos_x");
    node->add_input(DataType::Float, "pos_y");
    node->add_input(DataType::Float, "radius");

    node->add_output(DataType::Texture, "texture");
    return node;
  }
};


#endif //NAG_ENGINE_CIRCLE_NODE_H
