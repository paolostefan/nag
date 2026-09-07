#ifndef  NAG_ENGINE_NODES_GRADIENT_NODE_H
#define  NAG_ENGINE_NODES_GRADIENT_NODE_H

#include <memory>

#include "nlohmann/json.hpp"

#include "engine/nodes/shader_node.h"
#include "engine/visual_types.h"
#include "shaders/gradient_frag.h"

/**
 * @struct GradientNode
 * @brief Renders a gradient to texture.
 */
struct GradientNode : ShaderNode {
  enum class Type : uint8_t {
    Linear,
    Radial,
  };

  Type gradient_type{Type::Linear};
  Color color_start{1.f, 0.f, 0.f, 1.f};
  Color color_end{0.f, 0.f, 1.f, 1.f};
  Vec2 direction{1.f, 0.f};
  Vec2 center{0.5f, 0.5f};
  std::vector<Property> props_;

  GradientNode() {
    type = NodeType::Gradient;
    name = "Gradient";
    static constexpr const char *gradient_types[] = {"Linear", "Radial"};
    props_ = {
      MakeEnumProp(*this, &GradientNode::gradient_type, "gradient_type", "Gradient Type",
                   gradient_types, 2),
      MakeColorProp(*this, &GradientNode::color_start, "color_start", "Start Color"),
      MakeColorProp(*this, &GradientNode::color_end, "color_end", "End Color"),
    };
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Gradient"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "gradient"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kgradient_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    // Vec4 colors (ColorEdit rows) skip the base schema loop; Vec2 composites
    // have no schema row at all. Both are packed into arrays here.
    j["color_start"] = {color_start.x, color_start.y, color_start.z, color_start.w};
    j["color_end"] = {color_end.x, color_end.y, color_end.z, color_end.w};
    j["direction"] = {direction.x, direction.y};
    j["center"] = {center.x, center.y};
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      // Deserialize base class first
      if (auto result = VisualNode::deserialize_params(j); !result) {
        return result;
      }

      if (j.contains("color_start") && j["color_start"].is_array() && j["color_start"].size() >= 4) {
        color_start.x = j["color_start"][0];
        color_start.y = j["color_start"][1];
        color_start.z = j["color_start"][2];
        color_start.w = j["color_start"][3];
      }

      if (j.contains("color_end") && j["color_end"].is_array() && j["color_end"].size() >= 4) {
        color_end.x = j["color_end"][0];
        color_end.y = j["color_end"][1];
        color_end.z = j["color_end"][2];
        color_end.w = j["color_end"][3];
      }

      if (j.contains("direction") && j["direction"].is_array() && j["direction"].size() >= 2) {
        direction.x = j["direction"][0];
        direction.y = j["direction"][1];
      }

      if (j.contains("center") && j["center"].is_array() && j["center"].size() >= 2) {
        center.x = j["center"][0];
        center.y = j["center"][1];
      }

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize GradientNode params: ") + e.what()
      );
    }
  }

  void bind_params() override {
    shader->set_uniform("u_gradient_type", static_cast<int>(gradient_type));
    shader->set_uniform("u_color_start", color_start.x, color_start.y, color_start.z, color_start.w);
    shader->set_uniform("u_color_end", color_end.x, color_end.y, color_end.z, color_end.w);
    shader->set_uniform("u_direction", direction.x, direction.y);
    shader->set_uniform("u_center", center.x, center.y);
  }

  /**
  * Create a gradient node.
  */
  static std::unique_ptr<GradientNode> create(
    const Type gradient_type = Type::Linear,
    const Vec4 &color_start = Vec4::red(),
    const Vec4 &color_end = Vec4::blue()) {
    auto node = std::make_unique<GradientNode>();
    node->gradient_type = gradient_type;
    node->color_start = color_start;
    node->color_end = color_end;

    node->add_output(DataType::Texture, "texture");
    return node;
  }

  void update_from_inputs() override {
    // TODO: Add input connections for dynamic control
  }
};

#endif // NAG_ENGINE_NODES_GRADIENT_NODE_H
