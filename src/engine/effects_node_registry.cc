#include "engine/effect_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_effect_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<BlurNode>(
      NodeType::Blur,
      "Blur",
      "Effects",
      "Gaussian blur (two-pass separable)");

    registry.register_node<ChromaticAberrationNode>(
      NodeType::ChromaticAberration,
      "Chromatic Aberration",
      "Effects",
      "RGB channel offset — lens fringing");

    registry.register_node<PixelateNode>(
      NodeType::Pixelate,
      "Pixelate",
      "Effects",
      "Mosaic / pixelation effect");
  }
} // namespace node_registration
