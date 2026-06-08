#ifndef NAG_ENGINE_NODES_OUTPUT_NODE_H
#define NAG_ENGINE_NODES_OUTPUT_NODE_H

#include "engine/nodes/node.h"
#include "engine/render_target.h"

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
  [[nodiscard]] constexpr const Texture *get_texture() const { return last_texture_; }

private:
  const Texture *last_texture_{nullptr};
};

#endif //NAG_ENGINE_NODES_OUTPUT_NODE_H