#ifndef NAG_ENGINE_NODES_CLEAR_COLOR_NODE_H
#define NAG_ENGINE_NODES_CLEAR_COLOR_NODE_H

#include "engine/visual_types.h"
#include "engine/nodes/visual_node.h"
#include "engine/property_widget.h"

// ===========================================================================
// CLEAR COLOR NODE
// ===========================================================================

/**
 * Clears the render target with a solid color.
 * Useful as a background or for testing.
 */
struct ClearColorNode : VisualNode {
  Color color{0.f, 0.f, 0.f, 1.f};
  bool enabled{true};

  ClearColorNode() {
    type = NodeType::ClearColor;
    name = "ClearColor";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Clear Color"; }

  void render() override {
    static Color transparent = Color::transparent();
    if (!render_target || !render_target->is_valid()) {
      return;
    }

    update_from_inputs();

    render_target->bind();

    const Color *const clear_color = enabled ? &color : &transparent;

    glClearColor(clear_color->r(), clear_color->g(), clear_color->b(), clear_color->a());
    glClear(GL_COLOR_BUFFER_BIT);
    RenderTarget::unbind();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["color"] = {color.x, color.y, color.z, color.w};
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

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize ClearColorNode params: ") + e.what()
      );
    }
  }

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    static ImVec4 im_color{color.x, color.y, color.z, color.w};
    PropertyWidget::ColorEdit4(
      "Color",
      id,
      im_color,
      [](Node &n, const ImVec4 v) {
        auto &cnode = dynamic_cast<ClearColorNode &>(n);
        cnode.color = v;
      },
      graph, history
    );
  }

private:
  void update_from_inputs() {
    // Update color from inputs[0-3] if connected (r, g, b, a)
    if (inputs.size() >= 4) {
      for (int i = 0; i < 4; i++) {
        // Skip if not connected, the value will be taken from the node's own color parameter
        if (!inputs[i].connected) continue;

        if (const float *v = inputs[i].get_float()) {
          color[i] = *v;
        }
      }
    }

    if (inputs.size() >= 5) {
      // Update the 'enabled' field from inputs[4] if connected
      if (inputs[4].connected) {
        if (const bool *b = inputs[4].get_bool()) {
          enabled = *b;
        }
      }
    }
  }

public:
  /**
   * Create a clear color node.
   */
  static std::unique_ptr<ClearColorNode> create(const Vec4 &color = Vec4::black()) {
    auto node = std::make_unique<ClearColorNode>();

    node->color = color;
    node->add_input(DataType::Float, "r");
    node->add_input(DataType::Float, "g");
    node->add_input(DataType::Float, "b");
    node->add_input(DataType::Float, "a");
    node->add_input(DataType::Bool, "enabled");

    node->add_output(DataType::Texture, "texture");

    return node;
  }
};


#endif //NAG_ENGINE_NODES_CLEAR_COLOR_NODE_H
