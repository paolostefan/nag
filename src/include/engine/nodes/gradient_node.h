#ifndef  NAG_ENGINE_NODES_GRADIENT_NODE_H
#define  NAG_ENGINE_NODES_GRADIENT_NODE_H

#include <memory>

#include "editor/property_widget.h"
#include "nlohmann/json.hpp"

#include "engine/nodes/visual_node.h"
#include "engine/shader_manager.h"
#include "engine/shader_quad_helper.h"
#include "engine/visual_types.h"


// ===========================================================================
// GRADIENT NODE
// ===========================================================================

/**
 * Renders a gradient to texture.
 */
struct GradientNode : VisualNode {
  enum class Type : uint8_t {
    Linear,
    Radial,
  };

  Type gradient_type{Type::Linear};
  Color color_start{1.f, 0.f, 0.f, 1.f};
  Color color_end{0.f, 0.f, 1.f, 1.f};
  Vec2 direction{1.f, 0.f};
  Vec2 center{0.5f, 0.5f};

  std::shared_ptr<ShaderProgram> shader;

  GradientNode() {
    type = NodeType::Gradient;
    name = "Gradient";
  }

  bool initialize(const int width, const int height) override {
    if (!VisualNode::initialize(width, height)) {
      return false;
    }

    shader = ShaderManager::instance().load(
      "gradient",
      "shaders/fullscreen_quad.vert",
      "shaders/gradient.frag"
    );

    return shader && shader->is_valid();
  }

  void render() override {
    if (!render_target || !render_target->is_valid() || !shader || !shader->is_valid()) {
      return;
    }

    update_from_inputs();

    render_target->bind();
    render_target->clear(0.f, 0.f, 0.f, 0.f);

    shader->use();

    shader->set_uniform("u_resolution", render_target->get_fwidth(), render_target->get_fheight());
    shader->set_uniform("u_gradient_type", static_cast<int>(gradient_type));
    shader->set_uniform("u_color_start", color_start.x, color_start.y, color_start.z, color_start.w);
    shader->set_uniform("u_color_end", color_end.x, color_end.y, color_end.z, color_end.w);
    shader->set_uniform("u_direction", direction.x, direction.y);
    shader->set_uniform("u_center", center.x, center.y);

    ShaderQuadHelper::instance().render();

    ShaderProgram::unuse();

    RenderTarget::unbind();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["gradient_type"] = gradient_type;
    j["color_start"] = {color_start.x, color_start.y, color_start.z, color_start.w};
    j["color_end"] = {color_end.x, color_end.y, color_end.z, color_end.w};
    j["direction"] = {direction.x, direction.y};
    j["center"] = {center.x, center.y};
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      // Deserialize base class first
      auto result = VisualNode::deserialize_params(j);
      if (!result) {
        return result;
      }

      if (j.contains("gradient_type")) {
        gradient_type = static_cast<Type>(j["gradient_type"].get<int>());
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

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    static constexpr const char *types[] = {"Linear", "Radial", nullptr};
    // Gradient type
    int gradient_type_int = static_cast<int>(gradient_type);

    PropertyWidget::Combo("Gradient type",
                          /* node_id=*/ id,
                          /* value=*/ gradient_type_int,
                          /* items=*/ types,
                          /* item_count=*/ 2,
                          /* setter=*/[](Node &n, int v) {
                            dynamic_cast<GradientNode &>(n).gradient_type = static_cast<Type>(v);
                          },
                          graph, history);

    gradient_type = static_cast<Type>(gradient_type_int);

    auto *color_start_vec4 = reinterpret_cast<ImVec4 *>(&color_start);
    PropertyWidget::ColorEdit4("Start color",
                               /* node_id=*/ id,
                               /* value=*/ *color_start_vec4,
                               /* setter =*/[](Node &n, const ImVec4 &col) {
                                 dynamic_cast<GradientNode &>(n).color_start = col;
                               },
                               graph, history
    );

    auto *color_end_vec4 = reinterpret_cast<ImVec4 *>(&color_end);
    PropertyWidget::ColorEdit4("End color",
                               /* node_id=*/ id,
                               /* value=*/ *color_end_vec4,
                               /* setter =*/[](Node &n, const ImVec4 &col) {
                                 dynamic_cast<GradientNode &>(n).color_end = col;
                               },
                               graph, history
    );
  }

private:
  void update_from_inputs() {
    // TODO: Add input connections for dynamic control
  }

public:
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

    node->add_typed_output<Texture *>("texture");
    return node;
  }
};

#endif // NAG_ENGINE_NODES_GRADIENT_NODE_H
