#ifndef NAG_ENGINE_DISPLACE_NODE_H
#define NAG_ENGINE_DISPLACE_NODE_H

#include "engine/nodes/shader_node.h"
#include "shaders/displace_frag.h"

/**
 * @class DisplaceNode
 * @brief Distorts a source texture using a displacement map.
 *
 * Each pixel of the source is offset by an amount derived from
 * the RGB channels of the displacement map, remapped from [0,1] to [-1,1].
 * A neutral (no-displacement) map has mid-gray (0.5) in the selected channels.
 *
 * Inputs:
 *   texture    (Texture*) — source image to distort
 *   map        (Texture*) — displacement map
 *   strength   (float)    — maximum UV offset [0, 0.5]
 *
 * Output: Texture*
 *
 * @note channel_x and channel_y select which RGB channel drives each axis.
 *       0 = R, 1 = G, 2 = B. Exposed as GUI-only (not animatable pins).
 */
struct DisplaceNode : ShaderNode {
  float strength{0.05f};
  int channel_x{0}; // R drives X
  int channel_y{1}; // G drives Y
  std::vector<Property> props_;

  DisplaceNode() {
    type = NodeType::Displace;
    name = "Displace";
    static constexpr const char *channels[] = {"R", "G", "B"};
    props_ = {
      MakeFloatProp(*this, &DisplaceNode::strength, "strength", "Strength",
                    WidgetKind::SliderFloat, 0.f, 0.5f, "%.3f"),
      MakeIntProp(*this, &DisplaceNode::channel_x, "channel_x", "Channel X",
                  WidgetKind::Combo, 0.f, 0.f, "%d"),
      MakeIntProp(*this, &DisplaceNode::channel_y, "channel_y", "Channel Y",
                  WidgetKind::Combo, 0.f, 0.f, "%d"),
    };
    props_[0].disable_pin = "strength";
    props_[1].items = channels;
    props_[1].item_count = 3;
    props_[2].items = channels;
    props_[2].item_count = 3;
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Displace"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "displace"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kdisplace_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void bind_params() override {
    shader->set_uniform("u_strength", strength);
    shader->set_uniform("u_channel_x", static_cast<float>(channel_x));
    shader->set_uniform("u_channel_y", static_cast<float>(channel_y));
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<DisplaceNode> create(const float strength = 0.05f) {
    auto node = std::make_unique<DisplaceNode>();
    node->strength = strength;

    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Texture, "map");
    node->add_input(DataType::Float, "strength");

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

#endif  // NAG_ENGINE_DISPLACE_NODE_H
