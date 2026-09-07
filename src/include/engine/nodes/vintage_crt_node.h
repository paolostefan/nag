#ifndef NAG_ENGINE_NODES_VINTAGE_CRT_NODE_H
#define NAG_ENGINE_NODES_VINTAGE_CRT_NODE_H

#include "engine/nodes/shader_node.h"
#include "shaders/vintage_crt_frag.h"

// ============================================================================
// VintageCRTNode — pixelation / mosaic effect
// ============================================================================
//
// Quantises UVs to blocks of pixel_size x pixel_size pixels.
//
// Pins:
//   Input  0: Texture*  "texture"     — source image
//   Input  1: float     "pixel_size"  — block size in pixels [1..64], default 8.0
//   Output 0: Texture*  "texture"     — pixelated result
//
// Shader uniforms expected by shaders/pixelate.frag:
//   uniform sampler2D u_texture_0;
//   uniform float     u_pixel_size;
//   uniform vec2      u_resolution;
//
struct VintageCRTNode : ShaderNode {
  float pixel_size{8.f};
  std::vector<Property> props_;

  VintageCRTNode() {
    type = NodeType::VintageCRT;
    name = "Vintage CRT";
    props_ = {
      MakeFloatProp(*this, &VintageCRTNode::pixel_size, "pixel_size", "Pixel Size",
                    WidgetKind::SliderFloat, 1.f, 64.f, "%.0f"),
    };
    props_[0].disable_pin = "pixel_size";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Vintage CRT"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "vintage_crt"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kvintage_crt_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  [[nodiscard]] static std::unique_ptr<VintageCRTNode> create(
    const float pixel_size = 8.f) {
    auto node = std::make_unique<VintageCRTNode>();
    node->pixel_size = pixel_size;
    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "pixel_size");
    node->add_output(DataType::Texture, "texture");
    return node;
  }
};


#endif //NAG_ENGINE_NODES_VINTAGE_CRT_NODE_H
