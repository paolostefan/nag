#ifndef NAG_ENGINE_TRANSFORM_NODE_H
#define NAG_ENGINE_TRANSFORM_NODE_H

#include "engine/property_widget.h"
#include "engine/nodes/shader_node.h"
#include "shaders/transform_frag.h"

/**
 * @class TransformNode
 * @brief Applies 2D transform (translate, scale, rotate) to an input texture.
 *
 * The transform is computed in UV space around the center (0.5, 0.5).
 * Fragments mapping outside [0,1] after the inverse transform are transparent.
 *
 * Inputs:
 *   texture      (Texture*) — source image
 *   translate_x  (float)    — horizontal offset in normalized UV space
 *   translate_y  (float)    — vertical offset in normalized UV space
 *   scale        (float)    — uniform scale factor (1.0 = original size)
 *   rotation     (float)    — rotation in radians
 *
 * Output: Texture*
 */
struct TransformNode : ShaderNode {
  float translate_x{0.f};
  float translate_y{0.f};
  float scale{1.f};
  float rotation{0.f};

  TransformNode() {
    type = NodeType::Transform;
    name = "Transform";
  }

  [[nodiscard]] const char *shader_name()     const override { return "transform"; }
  [[nodiscard]] const char *frag_shader_src() const override { return ktransform_frag; }

  void bind_params() override {
    shader->set_uniform("u_translate_x", translate_x);
    shader->set_uniform("u_translate_y", translate_y);
    shader->set_uniform("u_scale",       scale);
    shader->set_uniform("u_rotation",    rotation);
  }

  // ── get_param — bridge for bind_float_inputs() ────────────────────────────

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "translate_x") return translate_x;
    if (param_name == "translate_y") return translate_y;
    if (param_name == "scale")       return scale;
    if (param_name == "rotation")    return rotation;
    return 0.f;
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["translate_x"] = translate_x;
    j["translate_y"] = translate_y;
    j["scale"]       = scale;
    j["rotation"]    = rotation;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (auto result = VisualNode::deserialize_params(j); !result) return result;

      if (j.contains("translate_x")) translate_x = j["translate_x"];
      if (j.contains("translate_y")) translate_y = j["translate_y"];
      if (j.contains("scale"))       scale        = j["scale"];
      if (j.contains("rotation"))    rotation     = j["rotation"];

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize TransformNode params: ") + e.what());
    }
  }

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    PropertyWidget::DragFloat("Translate X", id,
      translate_x,
      [](Node &n, const float v) { dynamic_cast<TransformNode &>(n).translate_x = v; },
      graph, history,
      /*speed=*/0.005f, /*min=*/-1.f, /*max=*/1.f,
      /*format=*/"%.3f",
      /*disabled=*/get_input("translate_x")->connected);

    PropertyWidget::DragFloat("Translate Y", id,
      translate_y,
      [](Node &n, const float v) { dynamic_cast<TransformNode &>(n).translate_y = v; },
      graph, history,
      /*speed=*/0.005f, /*min=*/-1.f, /*max=*/1.f,
      /*format=*/"%.3f",
      /*disabled=*/get_input("translate_y")->connected);

    PropertyWidget::DragFloat("Scale", id,
      scale,
      [](Node &n, const float v) { dynamic_cast<TransformNode &>(n).scale = v; },
      graph, history,
      /*speed=*/0.01f, /*min=*/0.01f, /*max=*/10.f,
      /*format=*/"%.3f",
      /*disabled=*/get_input("scale")->connected);

    PropertyWidget::DragFloat("Rotation", id,
      rotation,
      [](Node &n, const float v) { dynamic_cast<TransformNode &>(n).rotation = v; },
      graph, history,
      /*speed=*/0.01f, /*min=*/-3.14159f, /*max=*/3.14159f,
      /*format=*/"%.3f",
      /*disabled=*/get_input("rotation")->connected);
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<TransformNode> create(
      const float translate_x = 0.f,
      const float translate_y = 0.f,
      const float scale       = 1.f,
      const float rotation    = 0.f)
  {
    auto node         = std::make_unique<TransformNode>();
    node->translate_x = translate_x;
    node->translate_y = translate_y;
    node->scale       = scale;
    node->rotation    = rotation;

    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "translate_x");
    node->add_input(DataType::Float, "translate_y");
    node->add_input(DataType::Float, "scale");
    node->add_input(DataType::Float, "rotation");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

private:
  void update_from_inputs() override {
    auto read = [&](const char *pin_name, float &dst) {
      if (const Pin *p = get_input(pin_name); p && p->connected) {
        if (const float *s = p->get_float()) {
          dst = *s;
        }
      }
    };

    // inputs[0] is Texture* — handled by bind_texture_inputs() in ShaderNode::render()
    read("translate_x", translate_x);
    read("translate_y", translate_y);
    read("scale",       scale);
    read("rotation",    rotation);
  }
};

#endif  // NAG_ENGINE_TRANSFORM_NODE_H
