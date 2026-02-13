#ifndef NAG_ENGINE_VISUAL_NODES_H
#define NAG_ENGINE_VISUAL_NODES_H

#include <memory>

#include "spdlog/spdlog.h"

#include "engine/node.h"
#include "engine/render_target.h"
#include "engine/shader_manager.h"
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
    node->add_output("texture");

    return node;
  }
};


#endif //NAG_ENGINE_VISUAL_NODES_H
