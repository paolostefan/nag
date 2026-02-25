#include "engine/visual_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {

  void register_visual_nodes() {
    auto& registry = NodeRegistry::instance();

    registry.register_node<ClearColorNode>(
      NodeType::ClearColor,
      "Clear Color",
      "Visual",
      "Fills render target with solid color"
    );

    registry.register_node<GradientNode>(
      NodeType::Gradient,
      "Gradient",
      "Visual",
      "Renders linear or radial gradient"
    );

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

    registry.register_node<CompositeNode>(
      NodeType::Composite,
      "Composite",
      "Visual",
      "Composites multiple textures with blend modes"
    );
  }

}  // namespace node_registration