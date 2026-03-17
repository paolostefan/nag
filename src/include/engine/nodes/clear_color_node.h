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

  ClearColorNode() {
    type = NodeType::ClearColor;
    name = "ClearColor";
  }

  void render() override {
    if (!render_target || !render_target->is_valid()) {
      return;
    }

    update_from_inputs();

    render_target->bind();
    glClearColor(color.x, color.y, color.z, color.w);
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
      auto result = VisualNode::deserialize_params(j);
      if (!result) {
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
    ImVec4 im_color{color.x, color.y, color.z, color.w};
    PropertyWidget::ColorEdit4(
      "Color",
      id,
      im_color,
      [](Node &n, const ImVec4 v) {
        auto &cnode = dynamic_cast<ClearColorNode &>(n);
        cnode.color.x = v.x;
        cnode.color.y = v.y;
        cnode.color.z = v.z;
        cnode.color.w = v.w;
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

        if (const auto *stream = dynamic_cast<Stream<float> *>(inputs[i].stream.get())) {
          color[i] = stream->value;
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
    node->add_input("r");
    node->add_input("g");
    node->add_input("b");
    node->add_input("a");
    node->add_typed_output<Texture *>("texture");

    return node;
  }
};


#endif //NAG_ENGINE_NODES_CLEAR_COLOR_NODE_H
