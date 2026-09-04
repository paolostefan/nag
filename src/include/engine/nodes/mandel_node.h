#ifndef NAG_ENGINE_MANDEL_NODE_H
#define NAG_ENGINE_MANDEL_NODE_H

#include "shader_node.h"
#include "engine/visual_types.h"
#include "shaders/mandel_frag.h"

struct MandelNode : ShaderNode {
  Vec2 center{-1.f, 0.f};
  float zoom{.7f};
  int iterations{100};

  MandelNode() {
    type = NodeType::Mandel;
    name = "Mandelbrot";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Mandelbrot"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "mandelbrot"; }
  [[nodiscard]] constexpr const char *frag_shader_src() const noexcept override { return kmandel_frag; }

  static std::unique_ptr<MandelNode> create(const Vec2 &center_ = {-1.f, 0.f},
                                            const float zoom_ = .7f,
                                            const uint16_t iterations_ = 100) {
    auto node = std::make_unique<MandelNode>();
    node->center = center_;
    node->zoom = zoom_;
    node->iterations = iterations_;

    node->add_input(DataType::Float, "center_x");
    node->add_input(DataType::Float, "center_y");
    node->add_input(DataType::Float, "zoom");
    node->add_input(DataType::Int, "iterations");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

  void bind_params() override {
    shader->set_uniform("u_center_x", center.x);
    shader->set_uniform("u_center_y", center.y);
    shader->set_uniform("u_zoom", zoom);
    shader->set_uniform("u_iterations", iterations);
  }

  void update_from_inputs() override {
    if(const float *x = inputs[0].get_float()) {
      center.x = *x;
    }

    if(const float *y = inputs[1].get_float()) {
      center.y = *y;
    }

    if(const float *zoom_ptr = inputs[2].get_float()) {
      zoom = *zoom_ptr;
    }

    if(const int *it_ptr = inputs[3].get_int()) {
      iterations = *it_ptr;
    }
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();

    j["center"] = {center.x, center.y};
    j["zoom"] = zoom;
    j["iterations"] = iterations;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    if (auto result = VisualNode::deserialize_params(j); !result) return result;

    try {
      if (j.contains("center") && j["center"].is_array() && j["center"].size() >= 2) {
        center.x = j["center"][0];
        center.y = j["center"][1];
      }
      if (j.contains("zoom")) zoom = j["zoom"];
      if (j.contains("iterations")) iterations = j["iterations"];

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize Mandel params: ") + e.what());
    }
  }
};


#endif //NAG_ENGINE_MANDEL_NODE_H
