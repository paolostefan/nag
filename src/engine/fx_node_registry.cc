#include "engine/node_registry.h"
#include "engine/nodes/color_correction_node.h"
#include "engine/nodes/displace_node.h"
#include "engine/nodes/effect_nodes.h"
#include "engine/nodes/vintage_crt_node.h"

namespace node_registration {
  void register_fx_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node(NodeType::Blur, "Effects", "Gaussian blur (two-pass separable)", [] { return BlurNode::create(); });

    registry.register_node(NodeType::ChromaticAberration, "Effects", "RGB channel offset — lens fringing", [] { return ChromaticAberrationNode::create(); });

    registry.register_node(NodeType::ColorCorrection, "Effects", "Adjust brightness, contrast, saturation and hue of a texture", [] { return ColorCorrectionNode::create(); });

    registry.register_node(NodeType::Displace, "Effects", "Distort using a displacement map (grayscale texture input)", [] { return DisplaceNode::create(); });

    registry.register_node(NodeType::Pixelate, "Effects", "Mosaic / pixelation effect", [] { return PixelateNode::create(); });

    registry.register_node(NodeType::VintageCRT, "Effects", "Vintage TV / CRT effect with scanlines and chromatic aberration", [] { return VintageCRTNode::create(); });
  }
} // namespace node_registration
