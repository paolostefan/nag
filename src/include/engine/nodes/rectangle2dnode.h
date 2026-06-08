#ifndef NAG_ENGINE_RECTANGLE2DNODE_H
#define NAG_ENGINE_RECTANGLE2DNODE_H

#include "engine/nodes/shader_node.h"
#include "engine/visual_types.h"
#include "shaders/rectangle_frag.h"

/**
 * @struct Rectangle2DNode
 *
 * Renders a 2D rectangle using a fragment shader.
 */
struct Rectangle2DNode : ShaderNode {
  Vec2 position{0.5f, 0.5f}; // Center position [0,1]
  Vec2 size{0.3f, 0.2f}; // Width, height
  float rotation{0.f}; // Radians
  Color color{1.f, 1.f, 1.f, 1.f};
  float corner_radius{0.f}; // For rounded corners

  Rectangle2DNode() {
    type = NodeType::Rectangle2D;
    name = "Rectangle";
  }

  [[nodiscard]] const char *shader_name() const override { return "rectangle"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const override { return krectangle_frag; }

  void bind_params() override
  {
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_size", size.x, size.y);
    shader->set_uniform("u_rotation", rotation);
    shader->set_uniform("u_color", color.r(), color.g(), color.b(), color.a());
    shader->set_uniform("u_corner_radius", corner_radius);
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["position"] = {position.x, position.y};
    j["size"] = {size.x, size.y};
    j["rotation"] = rotation;
    j["color"] = {color.r(), color.g(), color.b(), color.a()};
    j["corner_radius"] = corner_radius;
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

      if (j.contains("size") && j["size"].is_array() && j["size"].size() >= 2) {
        size.x = j["size"][0];
        size.y = j["size"][1];
      }

      if (j.contains("rotation")) {
        rotation = j["rotation"];
      }

      if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 4) {
        color = Color(j["color"][0], j["color"][1], j["color"][2], j["color"][3]);
      }

      if (j.contains("corner_radius")) {
        corner_radius = j["corner_radius"];
      }

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize Rectangle2DNode params: ") + e.what()
      );
    }
  }

protected:
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
      if (const float *rot = inputs[2].get_float()) {
        rotation = *rot;
      }
    }
  }

public:
  /**
   * Create a rectangle node.
   */
  static std::unique_ptr<Rectangle2DNode> create(const Vec2 &position = {0.5f, 0.5f},
                                                 const Vec2 &size = {0.3f, 0.2f},
                                                 const Color &color = Color::white(),
                                                 const float corner_radius = 0.f) {
    auto node = std::make_unique<Rectangle2DNode>();
    node->position = position;
    node->size = size;
    node->color = color;
    node->corner_radius = corner_radius;

    node->add_input(DataType::Float, "pos_x");
    node->add_input(DataType::Float, "pos_y");
    node->add_input(DataType::Float, "rotation");

    node->add_output(DataType::Texture, "texture");

    return node;
  }

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    auto *color_ = reinterpret_cast<ImVec4 *>(&color);
    PropertyWidget::ColorEdit4("Color",
                               /* node_id=*/ id,
                               /* value=*/ *color_,
                               /* setter =*/[](Node &n, const ImVec4 &col) {
                                 dynamic_cast<Rectangle2DNode &>(n).color = col;
                               },
                               graph, history
    );

    // ------------------------------------------------------------------
    // Edge smoothness — slider with range [0, 1]
    // ------------------------------------------------------------------
    PropertyWidget::SliderFloat(
      "Corner radius",
      /*node_id=*/id,
      /*value=*/corner_radius,
      /*setter=*/[](Node &n, const float v) {
        dynamic_cast<Rectangle2DNode &>(n).corner_radius = v;
      },
      graph, history,
      /*min=*/0.f, /*max=*/1.f);
  }
};

#endif //NAG_ENGINE_RECTANGLE2DNODE_H
