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

  VintageCRTNode() {
    type = NodeType::VintageCRT;
    name = "Vintage CRT";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Vintage CRT"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "vintage_crt"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kvintage_crt_frag; }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["pixel_size"] = pixel_size;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    if (auto result = VisualNode::deserialize_params(j); !result) {
      return result;
    }

    if (j.contains("pixel_size")) pixel_size = j["pixel_size"];
    return OperationResult::ok();
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "pixel_size") return pixel_size;
    return 0.f;
  }

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
