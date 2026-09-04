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

  CompositeNode() {
    type = NodeType::Composite;
    name = CompositeNode::shader_name();
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Composite"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "composite"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kcomposite_frag; }

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
    node->add_input(DataType::Texture, "base"); // Texture input 0
    node->add_input(DataType::Texture, "blend"); // Texture input 1
    node->add_output(DataType::Texture, "texture"); // Composited output
    return node;
  }

};

#endif // NAG_ENGINE_NODES_COMPOSITE_NODE_H
