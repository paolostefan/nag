#ifndef NAG_ENGINE_BUMP_MAP_NODE_H
#define NAG_ENGINE_BUMP_MAP_NODE_H

#include "engine/nodes/shader_node.h"
#include "shaders/bump_map_frag.h"

/**
 * @class BumpMapNode
 * @brief Applies 2d height map to a 2d texture to create a bump-mapping effect.
 *
 * Inputs:
 *   texture  (Texture*) — source image to distort
 *   height   (Texture*) — displacement map
 *   light_x  (float)    - light direction X in [-1,1]
 *   light_y  (float)    - light direction Y in [-1,1]
 *   strength (float)    — height amplification
 *
 * Output: Texture*
 *
 * @note channel_x and channel_y select which RGB channel drives each axis.
 *       0 = R, 1 = G, 2 = B. Exposed as GUI-only (not animatable pins).
 */
struct BumpMapNode : ShaderNode {
  float light_x{-.5f};
  float light_y{.5f};
  float strength{8.f};
  float specular{.25f};

  std::vector<Property> props_;

  BumpMapNode() {
    type = NodeType::BumpMap;
    name = "BumpMap";
    props_ = {
      MakeFloatProp(*this, &BumpMapNode::light_x, "light_x", "Light X",
                  WidgetKind::SliderFloat, -1.f, 1.f, "%.3f"),
      MakeFloatProp(*this, &BumpMapNode::light_y, "light_y", "Light Y",
                  WidgetKind::SliderFloat, -1.f, 1.f, "%.3f"),
      MakeFloatProp(*this, &BumpMapNode::strength, "strength", "Strength",
                    WidgetKind::SliderFloat, 0.f, 32.f, "%.1f"),
      MakeFloatProp(*this, &BumpMapNode::specular, "specular", "Specular",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
    };

    props_[0].disable_pin = "light_x";
    props_[1].disable_pin = "light_y";
    props_[2].disable_pin = "strength";
    props_[3].disable_pin = "specular";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return name; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "bump_map"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kbump_map_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void bind_params() override {
    // placeholder
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<BumpMapNode> create() {
    auto node = std::make_unique<BumpMapNode>();

    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Texture, "height");
    node->add_input(DataType::Float, "light_x");
    node->add_input(DataType::Float, "light_y");
    node->add_input(DataType::Float, "strength");
    node->add_input(DataType::Float, "specular");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

  void update_from_inputs() override {
    // Texture* pins handled by bind_texture_inputs() in ShaderNode::render().
    if (const Pin *p = get_input("strength"); p && p->connected) {
      if (const float *s = p->get_float()) {
        strength = *s;
      }
    }
  }
};

#endif  // NAG_ENGINE_BUMP_MAP_NODE_H
