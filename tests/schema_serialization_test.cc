#include <gtest/gtest.h>
#include <SDL.h>

#include <memory>
#include <string>

#include "nlohmann/json.hpp"

#include "engine/node_registry.h"
#include "engine/nodes/circle_node.h"
#include "engine/nodes/clear_color_node.h"
#include "engine/nodes/color_correction_node.h"
#include "engine/nodes/composite_node.h"
#include "engine/nodes/displace_node.h"
#include "engine/nodes/ellipse_node.h"
#include "engine/nodes/generator_nodes.h"
#include "engine/nodes/gradient_node.h"
#include "engine/nodes/math_nodes.h"
#include "engine/nodes/particle_emitter_node.h"
#include "engine/nodes/particle_renderer_node.h"
#include "engine/nodes/polygon_node.h"
#include "engine/nodes/rectangle2dnode.h"
#include "engine/nodes/temporal_nodes.h"
#include "engine/nodes/texture_loader_node.h"
#include "engine/nodes/tile_node.h"
#include "engine/nodes/transform_node.h"
#include "engine/nodes/vintage_crt_node.h"

namespace {

/** SDL/GL context — some node deserialization (VisualNode) calls GL init. */
class SchemaFixture : public testing::Test {
protected:
  SDL_Window *window_{nullptr};
  SDL_GLContext gl_context_{nullptr};

  void SetUp() override {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    window_ = SDL_CreateWindow("Test", 0, 0, 1, 1,
                               SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    gl_context_ = SDL_GL_CreateContext(window_);
    glewInit();
    register_all_builtin_nodes();
  }

  void TearDown() override {
    if (gl_context_) SDL_GL_DeleteContext(gl_context_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
  }
};

/** Serialize a node's params and assert they EXACTLY equal @p expected. */
void expect_params(const Node &node, const nlohmann::json &expected) {
  auto actual = node.serialize_params();
  // width/height reflect transient FBO init state (512x512 after a
  // deserialize-triggered initialize), not schema params — drop both sides.
  actual.erase("width");
  actual.erase("height");
  auto exp = expected;
  exp.erase("width");
  exp.erase("height");
  EXPECT_EQ(actual, exp) << "type=" << node.type_name();
}

/** Deserialize @p json back into node and assert round-trip equality. */
void expect_round_trip(Node &node, const nlohmann::json &json) {
  ASSERT_TRUE(node.deserialize_params(json));
  expect_params(node, json);
}

} // namespace

// ============================================================================
// Schema-driven nodes (flat members, no per-node hooks)
// ============================================================================

TEST_F(SchemaFixture, ConstantFloatNodeSchemaFormat) {
  ConstantFloatNode node{0.f};
  node.value = 1.5f;
  expect_params(node,
                {{"gui_xy", {0.f, 0.f}},
                 {"value", 1.5f}});
}

TEST_F(SchemaFixture, NoiseNodeSchemaFormat) {
  auto node = NoiseNode::create();
  node->frequency = 2.f;
  node->amplitude = 1.5f;
  node->octaves = 4;
  node->persistence = 0.7f;
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"frequency", 2.f},
                 {"amplitude", 1.5f},
                 {"octaves", 4},
                 {"persistence", 0.7f}});
  expect_round_trip(*node, node->serialize_params());
}

TEST_F(SchemaFixture, RandomNodeSchemaFormat) {
  auto node = RandomNode::create();
  node->set_range(0.2f, 0.9f);
  node->set_seed(7);
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"min_value", 0.2f},
                 {"max_value", 0.9f},
                 {"seed", 7}});

  // Deserialize must rebuild the distribution via the side-effect setters.
  auto other = RandomNode::create(0.f, 1.f, 1);
  ASSERT_TRUE(other->deserialize_params(json));
  EXPECT_FLOAT_EQ(other->min_value, 0.2f);
  EXPECT_FLOAT_EQ(other->max_value, 0.9f);
  EXPECT_EQ(other->seed, 7);
}

TEST_F(SchemaFixture, LFONodeSchemaFormat) {
  auto node = LFONode::create();
  node->frequency = 0.5f;
  node->amplitude = 0.8f;
  node->wave_shape = LFONode::WaveShape::Triangle;
  node->phase = 1.f;
  node->offset = -0.5f;
  node->pulse_width = 0.6f;
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"frequency", 0.5f},
                 {"amplitude", 0.8f},
                 {"wave_shape", static_cast<int>(LFONode::WaveShape::Triangle)},
                 {"phase", 1.f},
                 {"offset", -0.5f},
                 {"pulse_width", 0.6f}});
  expect_round_trip(*node, node->serialize_params());
}

TEST_F(SchemaFixture, EnvelopeNodeSchemaFormat) {
  auto node = EnvelopeNode::create(0.05f, 0.2f, 0.9f, 0.3f);
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"attack_time", 0.05f},
                 {"decay_time", 0.2f},
                 {"sustain_level", 0.9f},
                 {"release_time", 0.3f}});
  expect_round_trip(*node, node->serialize_params());
}

TEST_F(SchemaFixture, DelayNodeSchemaFormat) {
  auto node = DelayNode::create();
  node->delay_time = 2.f;
  node->sample_rate = 90.f;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"delay_time", 2.f},
                 {"sample_rate", 90.f}});

  // Deserialize must resize the circular buffer via the side-effect setters.
  auto other = DelayNode::create();
  ASSERT_TRUE(other->deserialize_params(json));
  EXPECT_EQ(other->max_buffer_size, static_cast<size_t>(2.f * 90.f));
}

TEST_F(SchemaFixture, SmootherNodeSchemaFormat) {
  auto node = SmootherNode::create(0.25f);
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"smooth_time", 0.25f}});
  expect_round_trip(*node, node->serialize_params());
}

TEST_F(SchemaFixture, RemapNodeSchemaFormat) {
  auto node = RemapNode::create();
  node->in_min = -1.f;
  node->in_max = 1.f;
  node->out_min = 0.f;
  node->out_max = 10.f;
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"in_min", -1.f},
                 {"in_max", 1.f},
                 {"out_min", 0.f},
                 {"out_max", 10.f}});
  expect_round_trip(*node, node->serialize_params());
}

TEST_F(SchemaFixture, ClampNodeSchemaFormat) {
  auto node = ClampNode::create();
  node->min_value = -0.5f;
  node->max_value = 0.75f;
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"min_value", -0.5f},
                 {"max_value", 0.75f}});
  expect_round_trip(*node, node->serialize_params());
}

// ============================================================================
// Schema-driven ShaderNodes (VisualNode base adds enabled/width/height)
// ============================================================================

TEST_F(SchemaFixture, TileNodeSchemaFormat) {
  auto node = TileNode::create();
  node->tile_x = 3.f;
  node->tile_y = 2.f;
  node->offset_x = 0.1f;
  node->offset_y = 0.2f;
  node->mirror_x = true;
  node->mirror_y = false;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"tile_x", 3.f},
                 {"tile_y", 2.f},
                 {"offset_x", 0.1f},
                 {"offset_y", 0.2f},
                 {"mirror_x", true},
                 {"mirror_y", false},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, ColorCorrectionNodeSchemaFormat) {
  auto node = ColorCorrectionNode::create(0.1f, 1.2f, 0.5f, 0.3f);
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"brightness", 0.1f},
                 {"contrast", 1.2f},
                 {"saturation", 0.5f},
                 {"hue_shift", 0.3f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, CompositeNodeSchemaFormat) {
  auto node = CompositeNode::create(CompositeNode::BlendMode::Add, 0.5f);
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"blend_mode", static_cast<int>(CompositeNode::BlendMode::Add)},
                 {"opacity", 0.5f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, DisplaceNodeSchemaFormat) {
  auto node = DisplaceNode::create(0.1f);
  node->channel_x = 2;
  node->channel_y = 0;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"strength", 0.1f},
                 {"channel_x", 2},
                 {"channel_y", 0},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, TransformNodeSchemaFormat) {
  auto node = TransformNode::create(0.1f, 0.2f, 1.5f, 0.5f);
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"translate_x", 0.1f},
                 {"translate_y", 0.2f},
                 {"scale", 1.5f},
                 {"rotation", 0.5f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, VintageCRTNodeSchemaFormat) {
  auto node = VintageCRTNode::create(16.f);
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"pixel_size", 16.f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, ParticleRendererNodeSchemaFormat) {
  auto node = std::make_unique<ParticleRendererNode>();
  node->name = "Particle Renderer";
  node->color_jitter = 0.3f;
  node->alpha_jitter = 0.4f;
  node->size_min = 3.f;
  node->size_max = 8.f;
  node->size_scatter = 2.f;
  node->global_scale = 1.5f;
  node->emitter_x = 0.25f;
  node->emitter_y = 0.75f;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"color_jitter", 0.3f},
                 {"alpha_jitter", 0.4f},
                 {"size_min", 3.f},
                 {"size_max", 8.f},
                 {"size_scatter", 2.f},
                 {"global_scale", 1.5f},
                 {"emitter_x", 0.25f},
                 {"emitter_y", 0.75f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, ParticleEmitterNodeSchemaFormat) {
  ParticleEmitterNode node{7};
  node.rate = 20.f;
  node.speed = 250.f;
  node.min_life = 1.f;
  node.max_life = 3.f;
  node.max_particles = 5000;
  node.seed = 13;
  const auto json = node.serialize_params();
  expect_params(node,
                {{"gui_xy", {0.f, 0.f}},
                 {"rate", 20.f},
                 {"speed", 250.f},
                 {"min_life", 1.f},
                 {"max_life", 3.f},
                 {"max_particles", 5000},
                 {"seed", 13}});

  // Undo/redo setter path restores fields; seed setter must reseed the rng.
  ParticleEmitterNode other{99};
  other.seed = 99;
  other.rng.seed(99);
  ASSERT_TRUE(other.deserialize_params(json));
  EXPECT_EQ(other.max_particles, 5000u);
  EXPECT_EQ(other.seed, 13);
}

// ============================================================================
// Nodes with composite members (per-node pack/unpack hooks + schema rows)
// ============================================================================

TEST_F(SchemaFixture, ClearColorNodeSchemaFormat) {
  auto node = ClearColorNode::create(Vec4(0.1f, 0.2f, 0.3f, 0.4f));
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"color", std::vector<float>{0.1f, 0.2f, 0.3f, 0.4f}},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, GradientNodeSchemaFormat) {
  auto node = GradientNode::create(GradientNode::Type::Radial);
  node->color_start = Vec4(1.f, 0.f, 0.f, 1.f);
  node->color_end = Vec4(0.f, 0.f, 1.f, 0.5f);
  node->direction = {0.5f, 0.5f};
  node->center = {0.3f, 0.7f};
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"gradient_type", static_cast<int>(GradientNode::Type::Radial)},
                 {"color_start", std::vector<float>{1.f, 0.f, 0.f, 1.f}},
                 {"color_end", std::vector<float>{0.f, 0.f, 1.f, 0.5f}},
                 {"direction", std::vector<float>{0.5f, 0.5f}},
                 {"center", std::vector<float>{0.3f, 0.7f}},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, CircleNodeSchemaFormat) {
  auto node = CircleNode::create(Vec2(0.7f, 0.3f), 0.15f, Vec4(0.f, 1.f, 0.f, 0.8f));
  node->edge_smoothness = 0.02f;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"radius", 0.15f},
                 {"position", std::vector<float>{0.7f, 0.3f}},
                 {"color", std::vector<float>{0.f, 1.f, 0.f, 0.8f}},
                 {"edge_smoothness", 0.02f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, EllipseNodeSchemaFormat) {
  auto node = EllipseNode::create(Vec2(0.5f, 0.5f), 0.3f, 0.15f, Vec4::white());
  node->rotation = 0.5f;
  node->edge_smoothness = 0.03f;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"color", std::vector<float>{1.f, 1.f, 1.f, 1.f}},
                 {"radius_x", 0.3f},
                 {"radius_y", 0.15f},
                 {"rotation", 0.5f},
                 {"edge_smoothness", 0.03f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, Rectangle2DNodeSchemaFormat) {
  auto node = Rectangle2DNode::create(Vec2(0.5f, 0.5f), Vec2(0.4f, 0.3f),
                                      Color(1.f, 1.f, 0.f, 1.f));
  node->rotation = 0.2f;
  node->corner_radius = 0.05f;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"rotation", 0.2f},
                 {"corner_radius", 0.05f},
                 {"size", std::vector<float>{0.4f, 0.3f}},
                 {"color", std::vector<float>{1.f, 1.f, 0.f, 1.f}},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

TEST_F(SchemaFixture, PolygonNodeSchemaFormat) {
  auto node = PolygonNode::create(Vec2(0.5f, 0.5f), 0.2f, 5, Vec4::white());
  node->rotation = 0.3f;
  node->edge_smoothness = 0.04f;
  const auto json = node->serialize_params();
  expect_params(*node,
                {{"gui_xy", {0.f, 0.f}},
                 {"color", std::vector<float>{1.f, 1.f, 1.f, 1.f}},
                 {"n_sides", 5},
                 {"edge_smoothness", 0.04f},
                 {"radius", 0.2f},
                 {"rotation", 0.3f},
                 {"enabled", true}});
  expect_round_trip(*node, json);
}

// ============================================================================
// Widget-driven enum/combo lookup through get_param
// ============================================================================

TEST_F(SchemaFixture, SchemaGetParamFallsBackToSchemaRows) {
  auto lfo = LFONode::create();
  lfo->frequency = 3.f;
  EXPECT_FLOAT_EQ(lfo->get_param("frequency"), 3.f);

  auto composite = CompositeNode::create(CompositeNode::BlendMode::Screen, 0.3f);
  EXPECT_FLOAT_EQ(composite->get_param("opacity"), 0.3f);

  auto clamp = ClampNode::create(0.1f, 0.9f);
  EXPECT_FLOAT_EQ(clamp->get_param("min_value"), 0.1f);
  EXPECT_FLOAT_EQ(clamp->get_param("max_value"), 0.9f);
  EXPECT_FLOAT_EQ(clamp->get_param("bogus"), 0.f);
}

// ============================================================================
// Bespoke (non-schema) nodes must NOT gain a spurious schema
// ============================================================================

TEST_F(SchemaFixture, BespokeNodesHaveNoSchemaProperties) {
  auto texture = TextureLoaderNode::create();
  EXPECT_TRUE(texture->properties().empty());
}