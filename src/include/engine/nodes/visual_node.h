#ifndef NAG_ENGINE_VISUAL_NODE_H
#define NAG_ENGINE_VISUAL_NODE_H

#include "spdlog/spdlog.h"

#include "engine/nodes/node.h"
#include "engine/render_target.h"

/**
 * @struct VisualNode
 *
 * Base class for nodes that render to a texture via an FBO.
 * Manages the lifecycle of the render target and output texture stream.
 */
struct VisualNode : Node {
  std::unique_ptr<RenderTarget> render_target;
  Texture output_texture;
  bool enabled{true};

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
      render();
      if (!outputs.empty()) {
        outputs[0].set_texture(&output_texture);
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

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["enabled"] = enabled;

    // Serialize render target dimensions
    if (render_target && render_target->is_valid()) {
      j["width"] = render_target->get_width();
      j["height"] = render_target->get_height();
    }

    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {

    if (const OperationResult res = Node::deserialize_params(j); !res) {
      return res;
    }

    try {
      if (j.contains("enabled")) {
        enabled = j["enabled"];
      }

      // Render target will be re-initialized by derived classes
      // We just store the dimensions for later
      const int width = j.value("width", 512);
      const int height = j.value("height", 512);

      if (!initialize(width, height)) {
        return OperationResult::error("Failed to initialize render target");
      }

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize VisualNode params: ") + e.what()
      );
    }
  }
}; // struct VisualNode

#endif //NAG_ENGINE_VISUAL_NODE_H
