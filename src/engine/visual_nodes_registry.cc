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
#include "engine/nodes/sdf_shape_node.h"
#include "engine/nodes/texture_loader_node.h"
#include "engine/nodes/tile_node.h"

namespace node_registration {
  void register_visual_nodes() {
    auto &registry = NodeRegistry::instance();

    // ── Backgrounds ───────────────────────────────────────────────────────────

    registry.register_node<ClearColorNode>(
      NodeType::ClearColor,
      "Visual",
      "Fills render target with solid color"
    );

    registry.register_node<GradientNode>(
      NodeType::Gradient,
      "Visual",
      "Renders linear or radial gradient"
    );

    // ── Primitives ────────────────────────────────────────────────────────────

    registry.register_node<CircleNode>(
      NodeType::Circle,
      "Visual",
      "Renders a circle with smooth edges using SDF"
    );

    registry.register_node<EllipseNode>(
      NodeType::Ellipse,
      "Visual",
      "Renders an ellipse with independent x/y radii and rotation"
    );

    registry.register_node<PolygonNode>(
      NodeType::Polygon,
      "Visual",
      "Renders a regular n-gon using SDF"
    );

    registry.register_node<Rectangle2DNode>(
      NodeType::Rectangle2D,
      "Visual",
      "Renders a 2D rectangle with optional rounded corners"
    );

    registry.register_node<SDFShapeNode>(
      NodeType::SDFShape,
      "Visual",
      "Renders a shape defined by a signed distance function (SDF) shader"
    );

    registry.register_node<TextureLoaderNode>(
      NodeType::TextureLoader,
      "Visual",
      "Loads a texture from a saved picture file"
    );

    // ── Compositing ───────────────────────────────────────────────────────────

    registry.register_node<CompositeNode>(
      NodeType::Composite,
      "Visual",
      "Composites multiple textures with blend modes"
    );

    registry.register_node<TransformNode>(
      NodeType::Transform,
      "Visual",
      "Applies 2D translate, scale and rotation to a texture"
    );

    registry.register_node<TileNode>(
      NodeType::Tile,
      "Visual",
      "Tiles a texture"
    );

    // ── Output ────────────────────────────────────────────────────────────────

    registry.register_node<OutputNode>(
      NodeType::Output,
      "Visual",
      "Sink: displays the final texture"
    );
  }
} // namespace node_registration
