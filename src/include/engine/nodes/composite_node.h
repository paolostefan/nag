#ifndef  NAG_ENGINE_NODES_COMPOSITE_NODE_H
#define  NAG_ENGINE_NODES_COMPOSITE_NODE_H

#include "editor/property_widget.h"
#include "engine/nodes/visual_node.h"
#include "engine/shader_manager.h"
#include "engine/shader_quad_helper.h"

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

#endif // NAG_ENGINE_NODES_COMPOSITE_NODE_H
