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
#include "engine/nodes/particle_renderer_node.h"

namespace node_registration {
  void register_visual_nodes() {
    auto &registry = NodeRegistry::instance();

    // ── Backgrounds ───────────────────────────────────────────────────────────

    registry.register_node(NodeType::ClearColor, "Visual", "Fills render target with solid color", [] { return ClearColorNode::create(); });

    registry.register_node(NodeType::Gradient, "Visual", "Renders linear or radial gradient", [] { return GradientNode::create(); });

    // ── Primitives ────────────────────────────────────────────────────────────

    registry.register_node(NodeType::Circle, "Visual", "Renders a circle with smooth edges using SDF", [] { return CircleNode::create(); });

    registry.register_node(NodeType::Ellipse, "Visual", "Renders an ellipse with independent x/y radii and rotation", [] { return EllipseNode::create(); });

    registry.register_node(NodeType::Polygon, "Visual", "Renders a regular n-gon using SDF", [] { return PolygonNode::create(); });

    registry.register_node(NodeType::Rectangle2D, "Visual", "Renders a 2D rectangle with optional rounded corners", [] { return Rectangle2DNode::create(); });

    registry.register_node(NodeType::SDFShape, "Visual", "Renders a shape defined by a signed distance function (SDF) shader", [] { return SDFShapeNode::create(); });

    registry.register_node(NodeType::TextureLoader, "Visual", "Loads a texture from a saved picture file", [] { return TextureLoaderNode::create(); });

    // ── Compositing ───────────────────────────────────────────────────────────

    registry.register_node(NodeType::Composite, "Visual", "Composites multiple textures with blend modes", [] { return CompositeNode::create(); });

    registry.register_node(NodeType::Transform, "Visual", "Applies 2D translate, scale and rotation to a texture", [] { return TransformNode::create(); });

    registry.register_node(NodeType::Tile, "Visual", "Tiles a texture", [] { return TileNode::create(); });

    registry.register_node(NodeType::ParticleRenderer, "Visual", "Renders a 2D particle system", [] { return ParticleRendererNode::create(); });

    // ── Output ────────────────────────────────────────────────────────────────

    registry.register_node(NodeType::Output, "Visual", "Sink: displays the final texture", [] { return OutputNode::create(); });
  }
} // namespace node_registration
