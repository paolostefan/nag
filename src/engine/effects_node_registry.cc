#include "engine/node_registry.h"
#include "engine/nodes/circle_node.h"
#include "engine/nodes/effect_nodes.h"
#include "engine/nodes/rectangle2dnode.h"

namespace node_registration {
  void register_effect_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<CircleNode>(
      NodeType::Circle,
      "Circle",
      "Visual",
      "Renders a circle with smooth edges using SDF"
    );

    registry.register_node<Rectangle2DNode>(
      NodeType::Rectangle2D,
      "Rectangle",
      "Visual",
      "Renders a 2D rectangle with optional rounded corners"
    );

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
