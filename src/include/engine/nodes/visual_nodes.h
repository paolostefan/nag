#ifndef NAG_ENGINE_VISUAL_NODES_H
#define NAG_ENGINE_VISUAL_NODES_H

#include <memory>

#include "editor/property_widget.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "engine/nodes/visual_node.h"
#include "engine/shader_manager.h"
#include "engine/shader_quad_helper.h"
#include "engine/visual_types.h"


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

// ===========================================================================
// COMPOSITE NODE
// ===========================================================================

/**
 *  @class CompositeNode
 * Composites multiple texture inputs with blend modes.
 *
 * Defaults to 2 texture inputs and has always a texture output.
 */
struct CompositeNode : VisualNode {
  enum class BlendMode : uint8_t {
    Normal, // Alpha blend
    Add, // Additive
    Multiply,
    Screen,
  };

  BlendMode blend_mode{BlendMode::Normal};
  float opacity{1.f};

  std::shared_ptr<ShaderProgram> shader;

  CompositeNode() {
    type = NodeType::Composite;
    name = "Composite";
  }

  bool initialize(const int width, const int height) override {
    if (!VisualNode::initialize(width, height)) {
      return false;
    }

    shader = ShaderManager::instance().load(
      "composite",
      "shaders/fullscreen_quad.vert",
      "shaders/composite.frag"
    );

    return shader && shader->is_valid();
  }

  void render() override {
    if (!render_target || !render_target->is_valid() ||
        !shader || !shader->is_valid()) {
      return;
    }

    // Get input textures
    const Texture *base_tex = nullptr;
    const Texture *blend_tex = nullptr;

    if (inputs.size() >= 2) {
      if (const auto *const base_stream = dynamic_cast<Stream<Texture *> *>(inputs[0].stream.get())) {
        base_tex = base_stream->value;
      }
      if (const auto *const blend_stream = dynamic_cast<Stream<Texture *> *>(inputs[1].stream.get())) {
        blend_tex = blend_stream->value;
      }
    }

    if (!base_tex || !blend_tex || !base_tex->is_valid() || !blend_tex->is_valid()) {
      return;
    }

    render_target->bind();
    render_target->clear(0.f, 0.f, 0.f, 0.f);

    shader->use();

    shader->set_uniform("u_resolution", render_target->get_fwidth(), render_target->get_fheight());
    shader->set_uniform("u_blend_mode", static_cast<int>(blend_mode));
    shader->set_uniform("u_opacity", opacity);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, base_tex->texture_id);
    shader->set_uniform("u_texture_base", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blend_tex->texture_id);
    shader->set_uniform("u_texture_blend", 1);

    ShaderQuadHelper::instance().render();

    ShaderProgram::unuse();
    RenderTarget::unbind();

    // Unbind textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
  }
};

/**
 * @class OutputNode
 * @brief Sink node — receives the final Texture* and exposes it for display.
 *
 * Has a single typed input pin of type Texture*.
 * Does NOT own the texture; it only observes the pointer received via stream.
 * Use get_texture() to retrieve the current frame's result after evaluate().
 */
class OutputNode : public Node {
public:
  OutputNode() {
    name = "Output";
    type = NodeType::Output;
  }

  [[nodiscard]] static std::unique_ptr<OutputNode> create() {
    auto node = std::make_unique<OutputNode>();
    node->add_typed_input<Texture *>("texture");
    return node;
  }

  void evaluate() override {
    // Read the Texture* from the connected stream, if any.
    const auto *stream = dynamic_cast<Stream<Texture *> *>(inputs[0].stream.get());
    last_texture_ = stream != nullptr ? stream->value : nullptr;
    mark_inputs_consumed();
  }

  /**
   * @brief Returns the last texture received, or nullptr if not connected/evaluated.
   * The pointer is non-owning; valid only as long as the source VisualNode lives.
   */
  [[nodiscard]] const Texture *get_texture() const { return last_texture_; }

private:
  const Texture *last_texture_{nullptr};
};

#endif //NAG_ENGINE_VISUAL_NODES_H
