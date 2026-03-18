#include "engine/node_registry.h"
#include "engine/nodes/effect_nodes.h"

namespace node_registration {
  void register_fx_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<BlurNode>(
      NodeType::Blur,
      "Blur",
      "Effects",
      "Gaussian blur (two-pass separable)"
    );

    registry.register_node<ChromaticAberrationNode>(
      NodeType::ChromaticAberration,
      "Chromatic Aberration",
      "Effects",
      "RGB channel offset — lens fringing"
    );

    registry.register_node<PixelateNode>(
      NodeType::Pixelate,
      "Pixelate",
      "Effects",
      "Mosaic / pixelation effect"
    );
  }
} // namespace node_registration
