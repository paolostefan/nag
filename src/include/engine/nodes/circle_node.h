#ifndef NAG_ENGINE_CIRCLE_NODE_H
#define NAG_ENGINE_CIRCLE_NODE_H

#include "editor/property_widget.h"
#include "engine/nodes/shader_node.h"
#include "engine/visual_types.h"

/**
 * @class CircleNode
 * @brief Renders a circle to texture.
 */
struct CircleNode : ShaderNode {
  Vec2 position{0.5f, 0.5f};
  float radius{0.2f};
  Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
  float edge_smoothness{0.01f};

  CircleNode() {
    type = NodeType::Circle;
    name = "Circle";
  }

  [[nodiscard]] const char *shader_name() const override { return "circle"; }
  [[nodiscard]] const char *frag_shader_path() const override { return "shaders/circle.frag"; }

  void bind_params() override {
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_radius", radius);
    shader->set_uniform("u_color", color.x, color.y, color.z, color.w);
    shader->set_uniform("u_edge_smoothness", edge_smoothness);
  }

  void render() override {
    update_from_inputs();
    ShaderNode::render();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["position"] = {position.x, position.y};
    j["radius"] = radius;
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

      if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 2) {
        position.x = j["position"][0];
        position.y = j["position"][1];
      }

      if (j.contains("radius")) {
        radius = j["radius"];
      }

      if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 4) {
        color.x = j["color"][0];
        color.y = j["color"][1];
        color.z = j["color"][2];
        color.w = j["color"][3];
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
      /*min=*/0.0f, /*max=*/1.0f);
  }

private:
  void update_from_inputs() {
    if (inputs.size() >= 2) {
      if (const auto *x_stream = dynamic_cast<Stream<float> *>(inputs[0].stream.get())) {
        position.x = x_stream->value;
      }
      if (auto *y_stream = dynamic_cast<Stream<float> *>(inputs[1].stream.get())) {
        position.y = y_stream->value;
      }
    }

    if (inputs.size() >= 3) {
      if (const auto *r_stream = dynamic_cast<Stream<float> *>(inputs[2].stream.get())) {
        radius = r_stream->value;
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
    node->add_input("pos_x");
    node->add_input("pos_y");
    node->add_input("radius");

    node->add_typed_output<Texture *>("texture");
    return node;
  }
};


#endif //NAG_ENGINE_CIRCLE_NODE_H
