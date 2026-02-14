#ifndef NAG_ENGINE_VISUAL_NODES_H
#define NAG_ENGINE_VISUAL_NODES_H

#include <memory>

#include "spdlog/spdlog.h"

#include "engine/node.h"
#include "engine/render_target.h"
#include "engine/shader_manager.h"
#include "engine/shader_quad_helper.h"
#include "engine/visual_types.h"

struct VisualNode : Node {
  bool enabled{true};

  std::unique_ptr<RenderTarget> render_target;
  Texture output_texture;

  ~VisualNode() override = default;

  /**
   * Initialize render target with specified dimensions.
   */
  virtual bool initialize(const int width, const int height) {
    render_target = std::make_unique<RenderTarget>();

    if (!render_target->initialize(width, height)) {
      spdlog::error("Failed to initialize render target for {}", name);
      return false;
    }

    output_texture.texture_id = render_target->get_texture();
    output_texture.width = width;
    output_texture.height = height;

    return true;
  }

  /**
   * Render this visual node to its FBO
   */
  virtual void render() = 0;

  /**
   * Evaluate updates to output texture stream.
   */
  void evaluate() override {
    if (enabled && render_target && render_target->is_valid()) {
      // Render to FBO
      render();

      // Update the output stream with texture
      if (!outputs.empty()) {
        if (auto *tex_stream = dynamic_cast<Stream<Texture *> *>(outputs[0].stream.get())) {
          tex_stream->update(&output_texture);
        }
      }
    }

    mark_inputs_consumed();
  }

  /**
   * Resize the render target.
   */
  virtual void resize(const int width, const int height) {
    if (render_target) {
      render_target->resize(width, height);
      output_texture.texture_id = render_target->get_texture();
      output_texture.width = width;
      output_texture.height = height;
    }
  }
}; // struct VisualNode

// ===========================================================================
// CLEAR COLOR NODE
// ===========================================================================

/**
 * Clears the render target with a solid color.
 * Useful as a background or for testing.
 */
struct ClearColorNode : VisualNode {
  Vec4 color{0.0f, 0.0f, 0.0f, 1.0f};

  ClearColorNode() {
    type = ClearColor;
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

private:
  void update_from_inputs() {
    // Update color from inputs[0-3] if connected (r, g, b, a)
    if (inputs.size() >= 4) {
      if (const auto *r_stream = dynamic_cast<Stream<float> *>(inputs[0].stream.get())) {
        color.x = r_stream->value;
      }
      if (const auto *g_stream = dynamic_cast<Stream<float> *>(inputs[1].stream.get())) {
        color.y = g_stream->value;
      }
      if (const auto *b_stream = dynamic_cast<Stream<float> *>(inputs[2].stream.get())) {
        color.z = b_stream->value;
      }
      if (const auto *a_stream = dynamic_cast<Stream<float> *>(inputs[3].stream.get())) {
        color.w = a_stream->value;
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
  enum Type : uint8_t {
    Linear,
    Radial,
  };

  Type gradient_type{Type::Linear};
  Vec4 color_start{1.0f, 0.0f, 0.0f, 1.0f};
  Vec4 color_end{0.0f, 0.0f, 1.0f, 1.0f};
  Vec2 direction{1.0f, 0.0f};
  Vec2 center{0.5f, 0.5f};

  std::shared_ptr<ShaderProgram> shader;

  GradientNode() {
    type = Gradient;
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
    render_target->clear(0.0f, 0.0f, 0.0f, 0.0f);

    shader->use();

    shader->set_uniform("u_resolution", render_target->get_fwidth(), render_target->get_fheight());
    shader->set_uniform("u_gradient_type", static_cast<int>(gradient_type));
    shader->set_uniform("u_color_start", color_start.x, color_start.y, color_start.z, color_start.w);
    shader->set_uniform("u_color_end", color_end.x, color_end.y, color_end.z, color_end.w);
    shader->set_uniform("u_direction", direction.x, direction.y);
    shader->set_uniform("u_center", center.x, center.y);

    ShaderQuadHelper::instance().render();

    shader->unuse();
    RenderTarget::unbind();
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
// CIRCLE NODE
// ===========================================================================

/**
 * Renders a circle to texture.
 */
struct CircleNode : VisualNode {
  Vec2 position{0.5f, 0.5f};
  float radius{0.2f};
  Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
  float edge_smoothness{0.01f};

  std::shared_ptr<ShaderProgram> shader;

  CircleNode() {
    type = Circle;
    name = "Circle";
  }

  bool initialize(const int width, const int height) override {
    if (!VisualNode::initialize(width, height)) {
      return false;
    }

    shader = ShaderManager::instance().load(
      "circle",
      "shaders/fullscreen_quad.vert",
      "shaders/circle.frag"
    );

    return shader && shader->is_valid();
  }

  void render() override {
    if (!render_target || !render_target->is_valid() || !shader || !shader->is_valid()) {
      return;
    }

    update_from_inputs();

    render_target->bind();
    render_target->clear(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader->use();

    shader->set_uniform("u_resolution",
                        static_cast<float>(render_target->get_width()),
                        static_cast<float>(render_target->get_height()));
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_radius", radius);
    shader->set_uniform("u_color", color.x, color.y, color.z, color.w);
    shader->set_uniform("u_edge_smoothness", edge_smoothness);

    ShaderQuadHelper::instance().render();

    shader->unuse();

    glDisable(GL_BLEND);

    RenderTarget::unbind();
  }

private:
  void update_from_inputs() {
    if (inputs.size() >= 2) {
      if (const auto *x_stream = dynamic_cast<Stream<float> *>(inputs[0].stream.get())) {
        position.x = x_stream->value;
      }
      if (auto *y_stream = dynamic_cast<Stream<float> *>(inputs[1].stream.get())) {
        position.y = y_stream->value;
      }
    }

    if (inputs.size() >= 3) {
      if (const auto *r_stream = dynamic_cast<Stream<float> *>(inputs[2].stream.get())) {
        radius = r_stream->value;
      }
    }
  }

public:
  /**
   * Create a circle node.
   */
  static std::unique_ptr<CircleNode> create(const Vec2 &position = {0.5f, 0.5f},
                                            const float radius = 0.2f,
                                            const Vec4 &color = Vec4::white()) {
    auto node = std::make_unique<CircleNode>();
    node->position = position;
    node->radius = radius;
    node->color = color;
    node->add_input("pos_x");
    node->add_input("pos_y");
    node->add_input("radius");

    node->add_typed_output<Texture *>("texture");
    return node;
  }
};


// ===========================================================================
// RECTANGLE 2D NODE
// ===========================================================================

/**
 * Renders a 2D rectangle using a fragment shader.
 */
struct Rectangle2DNode : VisualNode {
  Vec2 position{0.5f, 0.5f}; // Center position [0,1]
  Vec2 size{0.3f, 0.2f}; // Width, height
  float rotation{0.0f}; // Radians
  Color color{1.0f, 1.0f, 1.0f, 1.0f};
  float corner_radius{0.0f}; // For rounded corners

  std::shared_ptr<ShaderProgram> shader;

  Rectangle2DNode() {
    type = Rectangle2D;
    name = "Rectangle";
  }

  bool initialize() {
    shader = ShaderManager::instance().load(
      "rectangle",
      "shaders/fullscreen_quad.vert",
      "shaders/rectangle.frag"
    );

    return shader && shader->is_valid();
  }

  void render() override {
    if (!render_target || !render_target->is_valid() || !shader || !shader->is_valid()) {
      return;
    }

    update_from_inputs();

    shader->use();

    shader->set_uniform("u_resolution", render_target->get_fwidth(), render_target->get_fheight());
    shader->set_uniform("u_position", position.x, position.y);
    shader->set_uniform("u_size", size.x, size.y);
    shader->set_uniform("u_rotation", rotation);
    shader->set_uniform("u_color", color.r(), color.g(), color.b(), color.a());
    shader->set_uniform("u_corner_radius", corner_radius);

    ShaderQuadHelper::instance().render();

    shader->unuse();
  }

private:
  void update_from_inputs() {
    if (inputs.size() >= 2) {
      if (const auto *const x_stream = dynamic_cast<Stream<float> *>(inputs[0].stream.get())) {
        position.x = x_stream->value;
      }
      if (const auto *const y_stream = dynamic_cast<Stream<float> *>(inputs[1].stream.get())) {
        position.y = y_stream->value;
      }
    }

    if (inputs.size() >= 3) {
      if (const auto *const rot_stream = dynamic_cast<Stream<float> *>(inputs[2].stream.get())) {
        rotation = rot_stream->value;
      }
    }
  }

public:
  /**
   * Create a rectangle node.
   */
  static std::unique_ptr<Rectangle2DNode> create(
    const Vec2 &position = {0.5f, 0.5f},
    const Vec2 &size = {0.3f, 0.2f},
    const Color &color = Color::white()) {
    auto node = std::make_unique<Rectangle2DNode>();
    node->position = position;
    node->size = size;
    node->color = color;

    node->add_input("pos_x");
    node->add_input("pos_y");
    node->add_input("rotation");

    node->add_typed_output<Texture *>("texture");

    return node;
  }
};


// ===========================================================================
// COMPOSITE NODE
// ===========================================================================

/**
 * Composites multiple texture inputs with blend modes.
 *
 * Defaults to 2 texture inputs and has always a texture output.
 */
struct CompositeNode : VisualNode {
  enum BlendMode : uint8_t {
    Normal, // Alpha blend
    Add, // Additive
    Multiply,
    Screen,
  };

  BlendMode blend_mode{Normal};
  float opacity{1.0f};

  std::shared_ptr<ShaderProgram> shader;

  CompositeNode() {
    type = Composite;
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
    if (!render_target || !render_target->is_valid() || !shader || !shader->is_valid()) {
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
    render_target->clear(0.0f, 0.0f, 0.0f, 0.0f);

    shader->use();

    shader->set_uniform("u_resolution",
                        static_cast<float>(render_target->get_width()),
                        static_cast<float>(render_target->get_height()));
    shader->set_uniform("u_blend_mode", blend_mode);
    shader->set_uniform("u_opacity", opacity);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, base_tex->texture_id);
    shader->set_uniform("u_texture_base", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blend_tex->texture_id);
    shader->set_uniform("u_texture_blend", 1);

    ShaderQuadHelper::instance().render();

    shader->unuse();
    RenderTarget::unbind();

    // Unbind textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  /**
   * Create a composite node.
   */
  static std::unique_ptr<CompositeNode> create(const BlendMode blend_mode = Normal,
                                               const float opacity = 1.0f) {
    auto node = std::make_unique<CompositeNode>();
    node->blend_mode = blend_mode;
    node->opacity = opacity;
    node->add_typed_input<Texture *>("base"); // Texture input 0
    node->add_typed_input<Texture *>("blend"); // Texture input 1
    node->add_typed_output<Texture *>("texture"); // Composited output
    return node;
  }
};

#endif //NAG_ENGINE_VISUAL_NODES_H
