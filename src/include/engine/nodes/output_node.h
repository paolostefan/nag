#ifndef NAG_ENGINE_NODES_OUTPUT_NODE_H
#define NAG_ENGINE_NODES_OUTPUT_NODE_H

#include "node.h"

/**
 * @class OutputNode
 * @brief Sink node — receives the final Texture* and exposes it for display.
 *
 * Has a single typed input pin of type Texture*.
 * Does NOT own the texture; it only observes the pointer received via stream.
 * Use get_texture() to retrieve the current frame's result after evaluate().
 */
struct OutputNode : Node {
  OutputNode() {
    name = "Output";
    type = NodeType::Output;
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Output"; }

  void evaluate() override {
    if (Texture **tex = inputs[0].get_texture()) {
      last_texture_ = *tex;
    }
    mark_inputs_consumed();
  }

  /**
   * @brief Returns the last texture received, or nullptr if not connected/evaluated.
   * The pointer is non-owning; valid only as long as the source VisualNode lives.
   */
  [[nodiscard]] constexpr const Texture *get_texture() const { return last_texture_; }

  [[nodiscard]] static std::unique_ptr<OutputNode> create() {
    auto node = std::make_unique<OutputNode>();
    node->add_input(DataType::Texture, "screen");
    return node;
  }

private:
  const Texture *last_texture_{nullptr};
};

#endif //NAG_ENGINE_NODES_OUTPUT_NODE_H
