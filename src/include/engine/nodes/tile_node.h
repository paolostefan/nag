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
  std::vector<Property> props_;

  TileNode() {
    type = NodeType::Tile;
    name = "Tile";
    props_ = {
      MakeFloatProp(*this, &TileNode::tile_x, "tile_x", "Tile X",
                    WidgetKind::DragFloat, 1.f, 32.f, "%.1f", SliderFlag::None),
      MakeFloatProp(*this, &TileNode::tile_y, "tile_y", "Tile Y",
                    WidgetKind::DragFloat, 1.f, 32.f, "%.1f", SliderFlag::None),
      MakeFloatProp(*this, &TileNode::offset_x, "offset_x", "Offset X",
                    WidgetKind::DragFloat, -1.f, 1.f, "%.3f", SliderFlag::None),
      MakeFloatProp(*this, &TileNode::offset_y, "offset_y", "Offset Y",
                    WidgetKind::DragFloat, -1.f, 1.f, "%.3f", SliderFlag::None),
      MakeBoolProp(*this, &TileNode::mirror_x, "mirror_x", "Mirror X"),
      MakeBoolProp(*this, &TileNode::mirror_y, "mirror_y", "Mirror Y"),
    };
    props_[0].disable_pin = "tile_x";
    props_[1].disable_pin = "tile_y";
    props_[2].disable_pin = "offset_x";
    props_[3].disable_pin = "offset_y";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Tile"; }
  [[nodiscard]] const char *shader_name() const noexcept override { return "tile"; }
  [[nodiscard]] const char *frag_shader_src() const noexcept override { return ktile_frag; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

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
