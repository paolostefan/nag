#include "engine/nodes/circle_node.h"
#include "engine/nodes/clear_color_node.h"
#include "engine/nodes/composite_node.h"
#include "engine/nodes/ellipse_node.h"
#include "engine/nodes/gradient_node.h"
#include "engine/nodes/output_node.h"
#include "engine/nodes/polygon_node.h"
#include "engine/nodes/rectangle2dnode.h"
#include "engine/nodes/transform_node.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_visual_nodes() {
    auto &registry = NodeRegistry::instance();

    // ── Backgrounds ───────────────────────────────────────────────────────────

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

    // ── Primitives ────────────────────────────────────────────────────────────

    registry.register_node<CircleNode>(
      NodeType::Circle,
      "Circle",
      "Visual",
      "Renders a circle with smooth edges using SDF"
    );

    registry.register_node<EllipseNode>(
      NodeType::Ellipse,
      "Ellipse",
      "Visual",
      "Renders an ellipse with independent x/y radii and rotation"
    );

    registry.register_node<PolygonNode>(
      NodeType::Polygon,
      "Polygon",
      "Visual",
      "Renders a regular n-gon using SDF"
    );

    registry.register_node<Rectangle2DNode>(
      NodeType::Rectangle2D,
      "Rectangle",
      "Visual",
      "Renders a 2D rectangle with optional rounded corners"
    );

    // ── Compositing ───────────────────────────────────────────────────────────

    registry.register_node<CompositeNode>(
      NodeType::Composite,
      "Composite",
      "Visual",
      "Composites multiple textures with blend modes"
    );

    registry.register_node<TransformNode>(
      NodeType::Transform,
      "Transform",
      "Visual",
      "Applies 2D translate, scale and rotation to a texture"
    );

    // ── Output ────────────────────────────────────────────────────────────────

    registry.register_node<OutputNode>(
      NodeType::Output,
      "Output",
      "Visual",
      "Sink: displays the final texture"
    );
  }
} // namespace node_registration