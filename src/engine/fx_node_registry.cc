#include "engine/node_registry.h"
#include "engine/nodes/color_correction_node.h"
#include "engine/nodes/displace_node.h"
#include "engine/nodes/effect_nodes.h"

namespace node_registration {
  void register_fx_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<BlurNode>(
      NodeType::Blur,
      "Effects",
      "Gaussian blur (two-pass separable)"
    );

    registry.register_node<ChromaticAberrationNode>(
      NodeType::ChromaticAberration,
      "Effects",
      "RGB channel offset — lens fringing"
    );

    registry.register_node<ColorCorrectionNode>(
      NodeType::ColorCorrection,
      "Effects",
      "Adjust brightness, contrast, saturation and hue of a texture"
    );

    registry.register_node<DisplaceNode>(
      NodeType::Displace,
      "Effects",
      "Distort using a displacement map (grayscale texture input)"
    );

    registry.register_node<PixelateNode>(
      NodeType::Pixelate,
      "Effects",
      "Mosaic / pixelation effect"
    );
  }
} // namespace node_registration
