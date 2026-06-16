#ifndef NAG_ENGINE_DISPLACE_NODE_H
#define NAG_ENGINE_DISPLACE_NODE_H

#include "engine/nodes/shader_node.h"
#include "engine/property_widget.h"
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

  DisplaceNode() {
    type = NodeType::Displace;
    name = "Displace";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Displace"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "displace"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return kdisplace_frag; }

  void bind_params() override {
    shader->set_uniform("u_strength", strength);
    shader->set_uniform("u_channel_x", static_cast<float>(channel_x));
    shader->set_uniform("u_channel_y", static_cast<float>(channel_y));
  }

  // ── get_param — bridge for bind_float_inputs() ────────────────────────────

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "strength") return strength;
    return 0.f;
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["strength"] = strength;
    j["channel_x"] = channel_x;
    j["channel_y"] = channel_y;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (auto result = VisualNode::deserialize_params(j); !result) return result;

      if (j.contains("strength")) strength = j["strength"];
      if (j.contains("channel_x")) channel_x = j["channel_x"];
      if (j.contains("channel_y")) channel_y = j["channel_y"];

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize DisplaceNode params: ") + e.what());
    }
  }

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    PropertyWidget::SliderFloat("Strength", id,
                                strength,
                                [](Node &n, const float v) { dynamic_cast<DisplaceNode &>(n).strength = v; },
                                graph, history,
                                /*min=*/0.f, /*max=*/0.5f, /*format=*/"%.3f",
                                /*disabled=*/get_input("strength")->connected);

    // Channel selectors — GUI only, not animatable
    constexpr const char *kChannels[] = {"R", "G", "B"};
    ImGui::Combo("X channel", &channel_x, kChannels, 3);
    ImGui::Combo("Y channel", &channel_y, kChannels, 3);
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
