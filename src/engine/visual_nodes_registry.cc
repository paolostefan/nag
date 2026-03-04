#include "engine/nodes/visual_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_visual_nodes() {
    auto &registry = NodeRegistry::instance();

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



    registry.register_node<CompositeNode>(
      NodeType::Composite,
      "Composite",
      "Visual",
      "Composites multiple textures with blend modes"
    );

    registry.register_node<OutputNode>(
      NodeType::Output,
      "Output",
      "Visual",
      "Sink: displays the final texture");
  }
} // namespace node_registration
