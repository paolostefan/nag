#ifndef NAG_ENGINE_NODES_TILE_NODE_H
#define NAG_ENGINE_NODES_TILE_NODE_H

#include "engine/nodes/shader_node.h"
#include "shaders/tile_frag.h"

/**
 * @class TileNode
 * @brief Tiles and/or mirrors an input texture with UV scroll support.
 *
 * Inputs:
 *   texture   (Texture*) — source image
 *   tile_x    (float)    — repetitions along X  [1, N]
 *   tile_y    (float)    — repetitions along Y  [1, N]
 *   offset_x  (float)    — UV scroll offset X   (animatable for scrolling fx)
 *   offset_y  (float)    — UV scroll offset Y
 *
 * Output: Texture*
 *
 * @note mirror_x / mirror_y are GUI-only toggles — alternating mirror/repeat
 *       per tile has no meaningful continuous analogue as a float pin.
 */
struct TileNode : ShaderNode {
  float tile_x{2.f};
  float tile_y{2.f};
  float offset_x{0.f};
  float offset_y{0.f};
  bool mirror_x{false};
  bool mirror_y{false};

  TileNode() {
    type = NodeType::Tile;
    name = "Tile";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Tile"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "tile"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return ktile_frag; }

  void bind_params() override {
    shader->set_uniform("u_tile_x", tile_x);
    shader->set_uniform("u_tile_y", tile_y);
    shader->set_uniform("u_offset_x", offset_x);
    shader->set_uniform("u_offset_y", offset_y);
    shader->set_uniform("u_mirror_x", mirror_x ? 1.f : 0.f);
    shader->set_uniform("u_mirror_y", mirror_y ? 1.f : 0.f);
  }

  void render() override {
    update_from_inputs();
    ShaderNode::render();
  }

  // ── get_param — bridge for bind_float_inputs() ────────────────────────────

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "tile_x") return tile_x;
    if (param_name == "tile_y") return tile_y;
    if (param_name == "offset_x") return offset_x;
    if (param_name == "offset_y") return offset_y;
    return 0.f;
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["tile_x"] = tile_x;
    j["tile_y"] = tile_y;
    j["offset_x"] = offset_x;
    j["offset_y"] = offset_y;
    j["mirror_x"] = mirror_x;
    j["mirror_y"] = mirror_y;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (auto result = VisualNode::deserialize_params(j); !result) return result;

      if (j.contains("tile_x")) tile_x = j["tile_x"];
      if (j.contains("tile_y")) tile_y = j["tile_y"];
      if (j.contains("offset_x")) offset_x = j["offset_x"];
      if (j.contains("offset_y")) offset_y = j["offset_y"];
      if (j.contains("mirror_x")) mirror_x = j["mirror_x"];
      if (j.contains("mirror_y")) mirror_y = j["mirror_y"];

      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize TileNode params: ") + e.what());
    }
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<TileNode> create(
    const float tile_x = 2.f,
    const float tile_y = 2.f,
    const bool mirror_x = false,
    const bool mirror_y = false) {
    auto node = std::make_unique<TileNode>();
    node->tile_x = tile_x;
    node->tile_y = tile_y;
    node->mirror_x = mirror_x;
    node->mirror_y = mirror_y;

    node->add_input(DataType::Texture, "texture");
    node->add_input(DataType::Float, "tile_x");
    node->add_input(DataType::Float, "tile_y");
    node->add_input(DataType::Float, "offset_x");
    node->add_input(DataType::Float, "offset_y");

    node->add_output(DataType::Texture, "texture");
    return node;
  }

  void update_from_inputs() override {
    read_pin_to("tile_x", tile_x);
    read_pin_to("tile_y", tile_y);
    read_pin_to("offset_x", offset_x);
    read_pin_to("offset_y", offset_y);
  }
};

#endif  // NAG_ENGINE_NODES_TILE_NODE_H
