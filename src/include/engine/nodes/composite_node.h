#ifndef  NAG_ENGINE_NODES_COMPOSITE_NODE_H
#define  NAG_ENGINE_NODES_COMPOSITE_NODE_H

#include <memory>

#include "editor/property_widget.h"
#include "engine/nodes/shader_node.h"
#include "shaders/composite_frag.h"

// ===========================================================================
// COMPOSITE NODE
// ===========================================================================

/**
 *  @class CompositeNode
 * Composites multiple texture inputs with blend modes.
 *
 * Defaults to 2 texture inputs and has always a texture output.
 */
struct CompositeNode : ShaderNode {
  enum class BlendMode : uint8_t {
    Normal, // Alpha blend
    Add, // Additive
    Multiply,
    Screen,
  };

  BlendMode blend_mode{BlendMode::Normal};
  float opacity{1.f};

  CompositeNode() {
    type = NodeType::Composite;
    name = CompositeNode::shader_name();
  }

  [[nodiscard]] const char *shader_name() const override {
    return "composite";
  }

  [[nodiscard]] constexpr const char *frag_shader_src() const override {
    return kcomposite_frag;
  }

  void bind_params() override {
    shader->set_uniform("u_blend_mode", static_cast<int>(blend_mode));
    shader->set_uniform("u_opacity", opacity);
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["blend_mode"] = blend_mode;
    j["opacity"] = opacity;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      // Deserialize base class first
      if (auto result = VisualNode::deserialize_params(j); !result) {
        return result;
      }

      if (j.contains("blend_mode")) {
        blend_mode = static_cast<BlendMode>(j["blend_mode"].get<int>());
      }

      if (j.contains("opacity")) {
        opacity = j["opacity"];
      }

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize CompositeNode params: ") + e.what()
      );
    }
  }

  /**
   * Create a composite node.
   */
  static std::unique_ptr<CompositeNode> create(const BlendMode blend_mode = BlendMode::Normal,
                                               const float opacity = 1.f) {
    auto node = std::make_unique<CompositeNode>();
    node->blend_mode = blend_mode;
    node->opacity = opacity;
    node->add_typed_input<Texture *>("base"); // Texture input 0
    node->add_typed_input<Texture *>("blend"); // Texture input 1
    node->add_typed_output<Texture *>("texture"); // Composited output
    return node;
  }

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    static constexpr const char *modes[] = {"Normal", "Add", "Multiply", "Screen", nullptr};

    // Blend mode
    int blend_mode_int = static_cast<int>(blend_mode);

    PropertyWidget::Combo("Blend mode",
                          /* node_id=*/ id,
                          /* value=*/ blend_mode_int,
                          /* items=*/ modes,
                          /* item_count=*/ 4,
                          /* setter=*/[](Node &n, int value) {
                            dynamic_cast<CompositeNode &>(n).blend_mode = static_cast<BlendMode>(value);
                          },
                          graph, history);

    blend_mode = static_cast<BlendMode>(blend_mode_int);

    // Opacity
    PropertyWidget::SliderFloat("Opacity",
                                /* node_id=*/ id,
                                /* value=*/ opacity,
                                /* setter=*/[](Node &n, float value) {
                                  dynamic_cast<CompositeNode &>(n).opacity = value;
                                },
                                graph, history,
                                /* min=*/ 0.f, /* max=*/ 1.f);
  }
};

#endif // NAG_ENGINE_NODES_COMPOSITE_NODE_H
