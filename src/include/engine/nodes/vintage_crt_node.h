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

  [[nodiscard]] const char *shader_name() const override { return "vintage_crt"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const override { return kvintage_crt_frag; }

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

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    PropertyWidget::SliderFloat(
      "Pixel Size",
      id,
      pixel_size,
      [](Node &n, const float v) { dynamic_cast<VintageCRTNode &>(n).pixel_size = v; },
      graph, history,
      1.f, 64.f, "%.0f"
    );
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "pixel_size") return pixel_size;
    return 0.f;
  }

  [[nodiscard]] static std::unique_ptr<VintageCRTNode> create(
    const float pixel_size = 8.f) {
    auto node = std::make_unique<VintageCRTNode>();
    node->pixel_size = pixel_size;
    node->add_typed_input<Texture *>("texture");
    node->add_typed_input<float>("pixel_size");
    node->add_typed_output<Texture *>("texture");
    return node;
  }
};


#endif //NAG_ENGINE_NODES_VINTAGE_CRT_NODE_H
