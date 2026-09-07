#ifndef  NAG_ENGINE_NODES_COMPOSITE_NODE_H
#define  NAG_ENGINE_NODES_COMPOSITE_NODE_H

#include <memory>

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
  std::vector<Property> props_;

  CompositeNode() {
    type = NodeType::Composite;
    name = CompositeNode::shader_name();
    static constexpr const char *blend_modes[] = {"Normal", "Add", "Multiply", "Screen"};
    props_ = {
      MakeEnumProp(*this, &CompositeNode::blend_mode, "blend_mode", "Blend Mode",
                   blend_modes, 4),
      MakeFloatProp(*this, &CompositeNode::opacity, "opacity", "Opacity",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
    };
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Composite"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "composite"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kcomposite_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void bind_params() override {
    shader->set_uniform("u_blend_mode", static_cast<int>(blend_mode));
    shader->set_uniform("u_opacity", opacity);
  }

  /**
   * Create a composite node.
   */
  static std::unique_ptr<CompositeNode> create(const BlendMode blend_mode = BlendMode::Normal,
                                               const float opacity = 1.f) {
    auto node = std::make_unique<CompositeNode>();
    node->blend_mode = blend_mode;
    node->opacity = opacity;
    node->add_input(DataType::Texture, "base"); // Texture input 0
    node->add_input(DataType::Texture, "blend"); // Texture input 1
    node->add_output(DataType::Texture, "texture"); // Composited output
    return node;
  }

};

#endif // NAG_ENGINE_NODES_COMPOSITE_NODE_H
