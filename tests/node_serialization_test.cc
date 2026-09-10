#include <gtest/gtest.h>
#include <SDL.h>

#include "engine/node_registry.h"
#include "engine/nodes/circle_node.h"
#include "engine/nodes/node.h"
#include "engine/nodes/generator_nodes.h"
#include "engine/nodes/visual_node.h"
#include "engine/nodes/clear_color_node.h"
#include "engine/nodes/composite_node.h"
#include "engine/nodes/gradient_node.h"
#include "engine/nodes/mandel_node.h"
#include "engine/nodes/math_nodes.h"
#include "engine/nodes/rectangle2dnode.h"
#include "engine/nodes/temporal_nodes.h"
#include "engine/serialization/json_graph_serializer.h"
#include "editor/graph_commands.h"

namespace fs = std::filesystem;

// ============================================================================
// Test Fixture – SDL/GL context needed for visual/MandelNode shader init
// ============================================================================
class NodeSerializationTest : public testing::Test {
protected:
  // ReSharper disable once CppDFATimeOver
  SDL_Window *window_{nullptr};
  SDL_GLContext gl_context_{nullptr};
  std::string test_file_path_;

  void SetUp() override {
    // SDL/GL context must be created BEFORE register_all_builtin_nodes()
    // because some node constructors (e.g. TextureLoaderNode) call OpenGL.
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    window_ = SDL_CreateWindow("Test", 0, 0, 1, 1,
                               SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    gl_context_ = SDL_GL_CreateContext(window_);
    glewInit();

    register_all_builtin_nodes();

    test_file_path_ = "test_graph.json";
  }

  void TearDown() override {
    // Cleanup test file
    if (!test_file_path_.empty() && fs::exists(test_file_path_)) {
      fs::remove(test_file_path_);
    }

    if (gl_context_) SDL_GL_DeleteContext(gl_context_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
  }
};


TEST_F(NodeSerializationTest, RenderTargetInitializes) {
  RenderTarget rt;
  EXPECT_TRUE(rt.initialize(256, 256, false));
}

// ============================================================================
// Test: Basic Save and Load
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadEmptyGraph) {
  const NodeGraph graph;
  const JsonGraphSerializer serializer;

  // Save empty graph
  const auto save_result = serializer.save(graph, test_file_path_);
  ASSERT_TRUE(save_result) << save_result.error_message;

  // Load it back
  NodeGraph loaded_graph;
  const auto load_result = serializer.load(loaded_graph, test_file_path_);
  ASSERT_TRUE(load_result) << load_result.error_message;

  EXPECT_EQ(loaded_graph.nodes.size(), 0);
  EXPECT_EQ(loaded_graph.links.size(), 0);
}

// ============================================================================
// Test: Save and Load Single Node
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadSingleNode) {
  NodeGraph graph;

  // Create LFO node
  auto lfo = LFONode::create(2.5f, 1.f, LFONode::WaveShape::Sawtooth, 0.5f, 0.1f);
  lfo->name = "Test LFO";
  lfo->gui_x = 100.f;
  lfo->gui_y = 200.f;
  const uint64_t original_id = lfo->id;

  graph.add_node(std::move(lfo));

  // Save
  JsonGraphSerializer serializer;
  auto save_result = serializer.save(graph, test_file_path_);
  ASSERT_TRUE(save_result) << save_result.error_message;

  // Load
  NodeGraph loaded_graph;
  auto load_result = serializer.load(loaded_graph, test_file_path_);
  ASSERT_TRUE(load_result) << load_result.error_message;

  // Verify
  ASSERT_EQ(loaded_graph.nodes.size(), 1);

  const auto *loaded_node = dynamic_cast<LFONode *>(loaded_graph.nodes[0].get());
  ASSERT_NE(loaded_node, nullptr);

  EXPECT_EQ(loaded_node->name, "Test LFO");
  EXPECT_EQ(loaded_node->type, NodeType::LFO);
  EXPECT_FLOAT_EQ(loaded_node->frequency, 2.5f);
  EXPECT_FLOAT_EQ(loaded_node->amplitude, 1.f);
  EXPECT_EQ(loaded_node->wave_shape, LFONode::WaveShape::Sawtooth);
  EXPECT_FLOAT_EQ(loaded_node->phase, 0.5f);
  EXPECT_FLOAT_EQ(loaded_node->offset, 0.1f);
  EXPECT_FLOAT_EQ(loaded_node->gui_x, 100.f);
  EXPECT_FLOAT_EQ(loaded_node->gui_y, 200.f);

  // ID should be different (regenerated)
  EXPECT_NE(loaded_node->id, original_id);
}

// ============================================================================
// Test: Save and Load Graph with Links
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadGraphWithLinks) {
  NodeGraph graph;

  // Create time node
  auto time_node = TimeNode::create();
  time_node->name = "Time";
  const Node *time_ptr = graph.add_node(std::move(time_node));

  // Create LFO connected to time
  auto lfo_node = LFONode::create(1.5f, 2.f);
  lfo_node->name = "LFO";
  const Node *lfo_ptr = graph.add_node(std::move(lfo_node));

  // Create smoother connected to LFO
  auto smoother_node = SmootherNode::create(0.2f);
  smoother_node->name = "Smoother";
  const Node *smoother_ptr = graph.add_node(std::move(smoother_node));

  // Create links
  graph.add_link(time_ptr->outputs[0], lfo_ptr->inputs[0]);
  graph.add_link(lfo_ptr->outputs[0], smoother_ptr->inputs[0]);
  graph.add_link(time_ptr->outputs[0], smoother_ptr->inputs[1]);

  ASSERT_EQ(graph.nodes.size(), 3);
  ASSERT_EQ(graph.links.size(), 3);

  // Save
  JsonGraphSerializer serializer;
  auto save_result = serializer.save(graph, test_file_path_);
  ASSERT_TRUE(save_result) << save_result.error_message;

  // Load
  NodeGraph loaded_graph;
  auto load_result = serializer.load(loaded_graph, test_file_path_);
  ASSERT_TRUE(load_result) << load_result.error_message;

  // Verify nodes
  ASSERT_EQ(loaded_graph.nodes.size(), 3);
  EXPECT_EQ(loaded_graph.nodes[0]->name, "Time");
  EXPECT_EQ(loaded_graph.nodes[1]->name, "LFO");
  EXPECT_EQ(loaded_graph.nodes[2]->name, "Smoother");

  // Verify links
  ASSERT_EQ(loaded_graph.links.size(), 3);

  // Verify connectivity (hard to check exact IDs, but structure should be preserved)
  const auto *loaded_lfo = loaded_graph.nodes[1].get();
  const auto *loaded_smoother = loaded_graph.nodes[2].get();

  EXPECT_NE(loaded_lfo->inputs[0].stream, nullptr);      // Connected to time
  EXPECT_NE(loaded_smoother->inputs[0].stream, nullptr); // Connected to LFO
  EXPECT_NE(loaded_smoother->inputs[1].stream, nullptr); // Connected to time
}

// ============================================================================
// Test: Multiple Parameter Types
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadEnvelopeNode) {
  NodeGraph graph;

  auto envelope = EnvelopeNode::create(
    0.15f,
    0.25f,
    0.8f,
    0.35f);
  envelope->name = "ADSR";
  graph.add_node(std::move(envelope));

  // Save and load
  JsonGraphSerializer serializer;
  ASSERT_TRUE(serializer.save(graph, test_file_path_));

  NodeGraph loaded_graph;
  ASSERT_TRUE(serializer.load(loaded_graph, test_file_path_));

  // Verify
  ASSERT_EQ(loaded_graph.nodes.size(), 1);
  const auto *loaded = dynamic_cast<EnvelopeNode *>(loaded_graph.nodes[0].get());
  ASSERT_NE(loaded, nullptr);

  EXPECT_FLOAT_EQ(loaded->attack_time, 0.15f);
  EXPECT_FLOAT_EQ(loaded->decay_time, 0.25f);
  EXPECT_FLOAT_EQ(loaded->sustain_level, 0.8f);
  EXPECT_FLOAT_EQ(loaded->release_time, 0.35f);
}

// ============================================================================
// Test: Error Handling - Invalid File
// ============================================================================
TEST_F(NodeSerializationTest, LoadFromNonExistentFile) {
  NodeGraph graph;
  const JsonGraphSerializer serializer;

  const auto result = serializer.load(graph, "nonexistent_file.json");
  EXPECT_FALSE(result);
  EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// Test: Error Handling - Corrupted JSON
// ============================================================================
TEST_F(NodeSerializationTest, LoadFromCorruptedFile) {
  // Write invalid JSON
  std::ofstream file(test_file_path_);
  file << "{ invalid json content";
  file.close();

  NodeGraph graph;
  JsonGraphSerializer serializer;

  auto result = serializer.load(graph, test_file_path_);
  EXPECT_FALSE(result);
}

// ============================================================================
// Test: ID Remapping
// ============================================================================
TEST_F(NodeSerializationTest, NodeIDsAreRemapped) {
  NodeGraph graph;

  auto node1 = LFONode::create();
  auto node2 = SmootherNode::create();

  const uint64_t original_id1 = node1->id;
  const uint64_t original_id2 = node2->id;

  graph.add_node(std::move(node1));
  graph.add_node(std::move(node2));

  // Save and load
  const JsonGraphSerializer serializer;
  ASSERT_TRUE(serializer.save(graph, test_file_path_));

  NodeGraph loaded_graph;
  ASSERT_TRUE(serializer.load(loaded_graph, test_file_path_));

  // IDs should be different (regenerated)
  EXPECT_NE(loaded_graph.nodes[0]->id, original_id1);
  EXPECT_NE(loaded_graph.nodes[1]->id, original_id2);

  // But loaded IDs should be unique among themselves
  EXPECT_NE(loaded_graph.nodes[0]->id, loaded_graph.nodes[1]->id);
}

// ============================================================================
// Test: Visual Node - ClearColor
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadClearColorNode) {
  NodeGraph graph;

  auto clear_node = ClearColorNode::create(Vec4(0.2f, 0.5f, 0.8f, 1.f));
  clear_node->name = "Background";

  bool init_result;

  // Initialize with specific dimensions to test render target serialization
  ASSERT_NO_FATAL_FAILURE(init_result = clear_node->initialize(640, 480));

  ASSERT_TRUE(init_result) << "Failed to initialize ClearColorNode";

  graph.add_node(std::move(clear_node));

  // Save
  JsonGraphSerializer serializer;
  auto save_result = serializer.save(graph, test_file_path_);
  ASSERT_TRUE(save_result) << save_result.error_message;

  // Load
  NodeGraph loaded_graph;
  auto load_result = serializer.load(loaded_graph, test_file_path_);
  ASSERT_TRUE(load_result) << load_result.error_message;

  // Verify
  ASSERT_EQ(loaded_graph.nodes.size(), 1);

  const auto *loaded_node = dynamic_cast<ClearColorNode *>(loaded_graph.nodes[0].get());
  ASSERT_NE(loaded_node, nullptr);

  EXPECT_EQ(loaded_node->name, "Background");
  EXPECT_FLOAT_EQ(loaded_node->color.x, 0.2f);
  EXPECT_FLOAT_EQ(loaded_node->color.y, 0.5f);
  EXPECT_FLOAT_EQ(loaded_node->color.z, 0.8f);
  EXPECT_FLOAT_EQ(loaded_node->color.w, 1.f);

  // Verify render target was recreated
  ASSERT_NE(loaded_node->render_target, nullptr);
  EXPECT_TRUE(loaded_node->render_target->is_valid());
  EXPECT_EQ(loaded_node->render_target->get_width(), 640);
  EXPECT_EQ(loaded_node->render_target->get_height(), 480);
}

// ============================================================================
// Test: Visual Node - Gradient
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadGradientNode) {
  NodeGraph graph;

  auto gradient = GradientNode::create(
    GradientNode::Type::Radial,
    Vec4(1.f, 0.f, 0.f, 1.f), // Red
    Vec4(0.f, 0.f, 1.f, 0.5f) // Semi-transparent blue
  );
  gradient->name = "Radial Gradient";
  gradient->center = {0.3f, 0.7f};
  gradient->direction = {0.5f, 0.5f};

  ASSERT_TRUE(gradient->initialize(800, 600));
  graph.add_node(std::move(gradient));

  // Save and load
  JsonGraphSerializer serializer;
  ASSERT_TRUE(serializer.save(graph, test_file_path_));

  NodeGraph loaded_graph;
  ASSERT_TRUE(serializer.load(loaded_graph, test_file_path_));

  // Verify
  ASSERT_EQ(loaded_graph.nodes.size(), 1);
  const auto *loaded = dynamic_cast<GradientNode *>(loaded_graph.nodes[0].get());
  ASSERT_NE(loaded, nullptr);

  EXPECT_EQ(loaded->gradient_type, GradientNode::Type::Radial);

  // Color start
  EXPECT_FLOAT_EQ(loaded->color_start.x, 1.f);
  EXPECT_FLOAT_EQ(loaded->color_start.y, 0.f);
  EXPECT_FLOAT_EQ(loaded->color_start.z, 0.f);
  EXPECT_FLOAT_EQ(loaded->color_start.w, 1.f);

  // Color end
  EXPECT_FLOAT_EQ(loaded->color_end.x, 0.f);
  EXPECT_FLOAT_EQ(loaded->color_end.y, 0.f);
  EXPECT_FLOAT_EQ(loaded->color_end.z, 1.f);
  EXPECT_FLOAT_EQ(loaded->color_end.w, 0.5f);

  // Center and direction
  EXPECT_FLOAT_EQ(loaded->center.x, 0.3f);
  EXPECT_FLOAT_EQ(loaded->center.y, 0.7f);
  EXPECT_FLOAT_EQ(loaded->direction.x, 0.5f);
  EXPECT_FLOAT_EQ(loaded->direction.y, 0.5f);

  // Shader should be reloaded
  ASSERT_NE(loaded->shader, nullptr);
  EXPECT_TRUE(loaded->shader->is_valid());
}

// ============================================================================
// Test: Visual Node - Circle
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadCircleNode) {
  NodeGraph graph;

  auto circle = CircleNode::create(
    Vec2(0.7f, 0.3f),         // position
    0.15f,                    // radius
    Vec4(0.f, 1.f, 0.f, 0.8f) // green semi-transparent
  );
  circle->name = "Moving Circle";
  circle->edge_smoothness = 0.02f;

  ASSERT_TRUE(circle->initialize(512, 512));
  graph.add_node(std::move(circle));

  // Save and load
  JsonGraphSerializer serializer;
  ASSERT_TRUE(serializer.save(graph, test_file_path_));

  NodeGraph loaded_graph;
  ASSERT_TRUE(serializer.load(loaded_graph, test_file_path_));

  // Verify
  ASSERT_EQ(loaded_graph.nodes.size(), 1);
  const auto *loaded = dynamic_cast<CircleNode *>(loaded_graph.nodes[0].get());
  ASSERT_NE(loaded, nullptr);

  EXPECT_FLOAT_EQ(loaded->position.x, 0.7f);
  EXPECT_FLOAT_EQ(loaded->position.y, 0.3f);
  EXPECT_FLOAT_EQ(loaded->radius, 0.15f);
  EXPECT_FLOAT_EQ(loaded->edge_smoothness, 0.02f);

  EXPECT_FLOAT_EQ(loaded->color.x, 0.f);
  EXPECT_FLOAT_EQ(loaded->color.y, 1.f);
  EXPECT_FLOAT_EQ(loaded->color.z, 0.f);
  EXPECT_FLOAT_EQ(loaded->color.w, 0.8f);
}

// ============================================================================
// Test: Visual Node - Rectangle
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadRectangleNode) {
  NodeGraph graph;

  auto rect = Rectangle2DNode::create(
    Vec2(0.5f, 0.5f),
    Vec2(0.4f, 0.3f),
    Color(1.f, 1.f, 0.f, 1.f)
  );
  rect->name = "Yellow Rectangle";
  rect->rotation = 0.785f; // 45 degrees
  rect->corner_radius = 0.05f;

  ASSERT_TRUE(rect->initialize(300, 300));
  graph.add_node(std::move(rect));

  // Save and load
  JsonGraphSerializer serializer;
  ASSERT_TRUE(serializer.save(graph, test_file_path_));

  NodeGraph loaded_graph;
  ASSERT_TRUE(serializer.load(loaded_graph, test_file_path_));

  // Verify
  ASSERT_EQ(loaded_graph.nodes.size(), 1);
  const auto *loaded = dynamic_cast<Rectangle2DNode *>(loaded_graph.nodes[0].get());
  ASSERT_NE(loaded, nullptr);

  EXPECT_FLOAT_EQ(loaded->position.x, 0.5f);
  EXPECT_FLOAT_EQ(loaded->position.y, 0.5f);
  EXPECT_FLOAT_EQ(loaded->size.x, 0.4f);
  EXPECT_FLOAT_EQ(loaded->size.y, 0.3f);
  EXPECT_FLOAT_EQ(loaded->rotation, 0.785f);
  EXPECT_FLOAT_EQ(loaded->corner_radius, 0.05f);

  EXPECT_FLOAT_EQ(loaded->color.r(), 1.f);
  EXPECT_FLOAT_EQ(loaded->color.g(), 1.f);
  EXPECT_FLOAT_EQ(loaded->color.b(), 0.f);
  EXPECT_FLOAT_EQ(loaded->color.a(), 1.f);
}

// ============================================================================
// Test: Visual Node - Composite
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadCompositeNode) {
  NodeGraph graph;

  auto composite = CompositeNode::create(
    CompositeNode::BlendMode::Multiply,
    0.75f
  );
  composite->name = "Multiply Blend";

  ASSERT_TRUE(composite->initialize(512, 512));
  graph.add_node(std::move(composite));

  // Save and load
  JsonGraphSerializer serializer;
  ASSERT_TRUE(serializer.save(graph, test_file_path_));

  NodeGraph loaded_graph;
  ASSERT_TRUE(serializer.load(loaded_graph, test_file_path_));

  // Verify
  ASSERT_EQ(loaded_graph.nodes.size(), 1);
  const auto *loaded = dynamic_cast<CompositeNode *>(loaded_graph.nodes[0].get());
  ASSERT_NE(loaded, nullptr);

  EXPECT_EQ(loaded->blend_mode, CompositeNode::BlendMode::Multiply);
  EXPECT_FLOAT_EQ(loaded->opacity, 0.75f);

  // Shader should be reloaded
  ASSERT_NE(loaded->shader, nullptr);
  EXPECT_TRUE(loaded->shader->is_valid());
}

// ============================================================================
// Test: Complex Visual Pipeline
// ============================================================================
TEST_F(NodeSerializationTest, SaveAndLoadVisualPipeline) {
  NodeGraph graph;

  // Create a pipeline: ClearColor -> Gradient -> Composite
  auto clear = ClearColorNode::create(Vec4::black());
  clear->name = "Black Background";
  ASSERT_TRUE(clear->initialize(800, 600));
  const Node *clear_ptr = graph.add_node(std::move(clear));

  auto gradient = GradientNode::create(
    GradientNode::Type::Linear,
    Vec4::red(),
    Vec4::blue()
  );
  gradient->name = "Red-Blue Gradient";
  ASSERT_TRUE(gradient->initialize(800, 600));
  const Node *grad_ptr = graph.add_node(std::move(gradient));

  auto composite = CompositeNode::create(
    CompositeNode::BlendMode::Add,
    1.f
  );
  composite->name = "Additive Composite";
  ASSERT_TRUE(composite->initialize(800, 600));
  const Node *comp_ptr = graph.add_node(std::move(composite));

  // Link: clear -> composite base, gradient -> composite blend
  graph.add_link(clear_ptr->outputs[0], comp_ptr->inputs[0]);
  graph.add_link(grad_ptr->outputs[0], comp_ptr->inputs[1]);

  ASSERT_EQ(graph.nodes.size(), 3);
  ASSERT_EQ(graph.links.size(), 2);

  // Save and load
  JsonGraphSerializer serializer;
  ASSERT_TRUE(serializer.save(graph, test_file_path_));

  NodeGraph loaded_graph;
  ASSERT_TRUE(serializer.load(loaded_graph, test_file_path_));

  // Verify structure
  ASSERT_EQ(loaded_graph.nodes.size(), 3);
  ASSERT_EQ(loaded_graph.links.size(), 2);

  // Verify node types
  EXPECT_NE(dynamic_cast<ClearColorNode*>(loaded_graph.nodes[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<GradientNode*>(loaded_graph.nodes[1].get()), nullptr);
  EXPECT_NE(dynamic_cast<CompositeNode*>(loaded_graph.nodes[2].get()), nullptr);

  // Verify connectivity
  const auto *loaded_composite = loaded_graph.nodes[2].get();
  EXPECT_NE(loaded_composite->inputs[0].stream, nullptr);
  EXPECT_NE(loaded_composite->inputs[1].stream, nullptr);
}

// Test that a MultiInputNode with >2 inputs gets (de)serialized properly
TEST_F(NodeSerializationTest, MultiInputNode) {
  const auto add3node = AddNode::create(3);

  ASSERT_NE(add3node, nullptr);

  const auto node_json = JsonGraphSerializer::serialize_node(add3node.get());

  ASSERT_TRUE(node_json.is_object());

  const auto deserialized = JsonGraphSerializer::deserialize_node(node_json);

  ASSERT_NE(deserialized, nullptr);

  ASSERT_EQ(deserialized->type, NodeType::Add);
  ASSERT_EQ(deserialized->inputs.size(), 3);
}


// ============================================================================
// Node (base class) – position & identity round-trip
// ============================================================================

TEST_F(NodeSerializationTest, NodeSerializeBaseParams) {
  const auto node = TimeNode::create();
  node->name = "MyTime";
  node->gui_x = 12.5f;
  node->gui_y = 34.7f;

  const auto j = JsonGraphSerializer::serialize_node(node.get());

  EXPECT_TRUE(j.contains("id"));
  EXPECT_EQ(j["type"], "Time");
  EXPECT_EQ(j["name"], "MyTime");
  EXPECT_FLOAT_EQ(j["gui_xy"][0], 12.5f);
  EXPECT_FLOAT_EQ(j["gui_xy"][1], 34.7f);
  EXPECT_TRUE(j.contains("params"));
}

TEST_F(NodeSerializationTest, NodeRoundTripPreservesPositionAndName) {
  const auto node = ConstantFloatNode::create(3.14f);
  node->name = "Pi";
  node->gui_x = 320.f;
  node->gui_y = 240.f;

  const auto j = JsonGraphSerializer::serialize_node(node.get());
  const auto loaded = JsonGraphSerializer::deserialize_node(j);
  ASSERT_NE(loaded, nullptr);

  EXPECT_EQ(loaded->name, "Pi");
  EXPECT_EQ(loaded->type, NodeType::Constant);
  EXPECT_FLOAT_EQ(loaded->gui_x, 320.f);
  EXPECT_FLOAT_EQ(loaded->gui_y, 240.f);
}

TEST_F(NodeSerializationTest, NodeMissingPositionDefaultsToZero) {
  nlohmann::json j;
  j["type"] = "Constant";
  j["name"] = "NoPos";
  j["position"] = nlohmann::json::array(); // empty — no usable coords
  j["params"] = nlohmann::json::object();

  auto loaded = JsonGraphSerializer::deserialize_node(j);
  ASSERT_NE(loaded, nullptr);
  EXPECT_FLOAT_EQ(loaded->gui_x, 0.f);
  EXPECT_FLOAT_EQ(loaded->gui_y, 0.f);
}

// ============================================================================
// VisualNode – enabled / render-target dimensions
// ============================================================================

TEST_F(NodeSerializationTest, VisualNodeSerializesEnabledAndDimensions) {
  const auto node = ClearColorNode::create();
  ASSERT_TRUE(node->initialize(640, 480));

  const auto j = node->serialize_params();

  EXPECT_TRUE(j["enabled"].get<bool>());
  EXPECT_EQ(j["width"].get<int>(), 640);
  EXPECT_EQ(j["height"].get<int>(), 480);
}

TEST_F(NodeSerializationTest, VisualNodeUninitializedOmitsDimensions) {
  auto node = ClearColorNode::create();

  const auto j = node->serialize_params();

  EXPECT_TRUE(j["enabled"].get<bool>());
  EXPECT_FALSE(j.contains("width"));
  EXPECT_FALSE(j.contains("height"));
}

TEST_F(NodeSerializationTest, VisualNodeDeserializeRestoresDisabledState) {
  nlohmann::json j;
  j["enabled"] = false;

  auto node = ClearColorNode::create();
  const auto result = node->deserialize_params(j);
  ASSERT_TRUE(result) << result.error_message;

  // VisualNode::enabled is shadowed by ClearColorNode::enabled — check
  // the base member via explicit scope to verify deserialization.
  EXPECT_FALSE(static_cast<VisualNode *>(node.get())->enabled);
  ASSERT_NE(node->render_target, nullptr);
  EXPECT_TRUE(node->render_target->is_valid());
  // Default dimensions when width/height omitted
  EXPECT_EQ(node->render_target->get_width(), 512);
  EXPECT_EQ(node->render_target->get_height(), 512);
}

TEST_F(NodeSerializationTest, VisualNodeDeserializeCustomDimensions) {
  nlohmann::json j;
  j["enabled"] = true;
  j["width"] = 800;
  j["height"] = 600;

  auto node = ClearColorNode::create();
  const auto result = node->deserialize_params(j);
  ASSERT_TRUE(result) << result.error_message;

  ASSERT_NE(node->render_target, nullptr);
  EXPECT_EQ(node->render_target->get_width(), 800);
  EXPECT_EQ(node->render_target->get_height(), 600);
}

// ============================================================================
// MandelNode – center, zoom, iterations
// ============================================================================

TEST_F(NodeSerializationTest, MandelNodeSerializesAllParams) {
  auto node = MandelNode::create(
    Vec2(0.5f, -0.5f),
    1.5f,
    200
  );
  ASSERT_TRUE(node->initialize(1024, 768));

  const auto j = node->serialize_params();

  // Inherited from VisualNode
  EXPECT_TRUE(j["enabled"].get<bool>());
  EXPECT_EQ(j["width"].get<int>(), 1024);
  EXPECT_EQ(j["height"].get<int>(), 768);

  // Mandel-specific
  EXPECT_FLOAT_EQ(j["center"][0], 0.5f);
  EXPECT_FLOAT_EQ(j["center"][1], -0.5f);
  EXPECT_FLOAT_EQ(j["zoom"], 1.5f);
  EXPECT_EQ(j["iterations"].get<int>(), 200);
}

TEST_F(NodeSerializationTest, MandelNodeRoundTripPreservesValues) {
  auto node = MandelNode::create(
    Vec2(-0.75f, 0.25f),
    0.5f,
    500
  );
  node->name = "DeepZoom";
  node->gui_x = 100.f;
  node->gui_y = 50.f;
  ASSERT_TRUE(node->initialize(800, 600));

  const auto node_json = JsonGraphSerializer::serialize_node(node.get());
  auto loaded = JsonGraphSerializer::deserialize_node(node_json);
  ASSERT_NE(loaded, nullptr);

  EXPECT_EQ(loaded->name, "DeepZoom");
  EXPECT_EQ(loaded->type, NodeType::Mandel);
  EXPECT_FLOAT_EQ(loaded->gui_x, 100.f);
  EXPECT_FLOAT_EQ(loaded->gui_y, 50.f);

  auto *mandel = dynamic_cast<MandelNode *>(loaded.get());
  ASSERT_NE(mandel, nullptr);

  EXPECT_FLOAT_EQ(mandel->center.x, -0.75f);
  EXPECT_FLOAT_EQ(mandel->center.y, 0.25f);
  EXPECT_FLOAT_EQ(mandel->zoom, 0.5f);
  EXPECT_EQ(mandel->iterations, 500);

  // Render target recreated
  ASSERT_NE(mandel->render_target, nullptr);
  EXPECT_TRUE(mandel->render_target->is_valid());
  EXPECT_EQ(mandel->render_target->get_width(), 800);
  EXPECT_EQ(mandel->render_target->get_height(), 600);

  // Shader reloaded
  ASSERT_NE(mandel->shader, nullptr);
  EXPECT_TRUE(mandel->shader->is_valid());
}

TEST_F(NodeSerializationTest, MandelNodeDeserializeMissingFieldsUseDefaults) {
  // Minimal JSON – only VisualNode fields, no Mandel-specific keys
  nlohmann::json j;
  j["enabled"] = true;
  j["width"] = 320;
  j["height"] = 240;

  auto node = MandelNode::create();
  const auto result = node->deserialize_params(j);
  ASSERT_TRUE(result) << result.error_message;

  // Should retain the member-initialiser defaults
  EXPECT_FLOAT_EQ(node->center.x, -1.f);
  EXPECT_FLOAT_EQ(node->center.y, 0.f);
  EXPECT_FLOAT_EQ(node->zoom, 0.7f);
  EXPECT_EQ(node->iterations, 100);
}

TEST_F(NodeSerializationTest, MandelNodeSerializeWithoutInitOmitsDimensions) {
  const auto node = MandelNode::create(
    Vec2(0.1f, -0.2f),
    2.f,
    50
  );

  const auto j = node->serialize_params();

  // VisualNode fields without render-target
  EXPECT_TRUE(j["enabled"].get<bool>());
  EXPECT_FALSE(j.contains("width"));
  EXPECT_FALSE(j.contains("height"));

  // Mandel fields still present
  EXPECT_FLOAT_EQ(j["center"][0], 0.1f);
  EXPECT_FLOAT_EQ(j["center"][1], -0.2f);
  EXPECT_FLOAT_EQ(j["zoom"], 2.f);
  EXPECT_EQ(j["iterations"].get<int>(), 50);
}

// ============================================================================
// Single canonical node adapter (NodeRegistry) — interrogation + round trips
// ============================================================================

// The registry adapter is the single source of truth: it must write the same
// JSON the serializer does, and always use the stable string "type".
TEST_F(NodeSerializationTest, RegistryAdapterIsCanonicalSingleFormat) {
  auto node = LFONode::create(2.f, 0.5f);
  node->name = "Canonical";
  node->gui_x = 42.f;
  node->gui_y = 24.f;

  const auto via_serializer = JsonGraphSerializer::serialize_node(node.get());
  const auto via_registry = NodeRegistry::instance().serialize_node(*node);

  EXPECT_EQ(via_registry, via_serializer);
  ASSERT_TRUE(via_registry.contains("type"));
  EXPECT_TRUE(via_registry["type"].is_string());
  EXPECT_EQ(via_registry["type"], "LFO");
}

// Old saves store "type" as the enum integer; the adapter must keep reading
// them while writing the stable string form going forward.
TEST_F(NodeSerializationTest, RegistryAdapterAcceptsLegacyNumericType) {
  nlohmann::json j;
  j["id"] = 7;
  j["type"] = static_cast<int>(NodeType::Constant);
  j["name"] = "OldSave";
  j["gui_xy"] = {10.f, 20.f};
  j["params"] = {{"value", 7.0f}};

  auto loaded = NodeRegistry::instance().deserialize_node(j);
  ASSERT_NE(loaded, nullptr);
  EXPECT_EQ(loaded->type, NodeType::Constant);
  EXPECT_EQ(loaded->name, "OldSave");
  EXPECT_FLOAT_EQ(loaded->gui_x, 10.f);
  EXPECT_FLOAT_EQ(loaded->gui_y, 20.f);

  auto *constant = dynamic_cast<ConstantFloatNode *>(loaded.get());
  ASSERT_NE(constant, nullptr);
  EXPECT_FLOAT_EQ(constant->value, 7.0f);
}

// Unknown string types must fail cleanly (nullptr), not crash.
TEST_F(NodeSerializationTest, RegistryAdapterRejectsUnknownType) {
  nlohmann::json j;
  j["type"] = "DefinitelyNotANode";
  j["params"] = nlohmann::json::object();

  EXPECT_EQ(NodeRegistry::instance().deserialize_node(j), nullptr);
}

// AddNodeCommand redo path deserializes its snapshot through the adapter.
TEST_F(NodeSerializationTest, AddNodeCommandRedoUsesCanonicalFormat) {
  NodeGraph graph;

  auto node = ConstantFloatNode::create(1.25f);
  node->name = "RedoMe";
  AddNodeCommand cmd(std::move(node));

  ASSERT_TRUE(cmd.execute(graph)); // first run: moves the node in
  ASSERT_EQ(graph.nodes.size(), 1);

  // Redo: node_ is exhausted, the stored snapshot is deserialized back
  ASSERT_TRUE(cmd.execute(graph));
  ASSERT_EQ(graph.nodes.size(), 2);

  auto *restored = dynamic_cast<ConstantFloatNode *>(graph.nodes[1].get());
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->type, NodeType::Constant);
  EXPECT_EQ(restored->name, "RedoMe");
  EXPECT_FLOAT_EQ(restored->value, 1.25f);
}

// DeleteNodesCommand undo restores deleted nodes through the shared adapter.
TEST_F(NodeSerializationTest, DeleteNodesCommandRestoresNodeFromCanonicalFormat) {
  NodeGraph graph;

  auto node = ConstantFloatNode::create(2.5f);
  node->name = "DeleteMe";
  node->gui_y = 6.f;
  node->gui_x = 5.f;
  const auto *added = graph.add_node(std::move(node));

  DeleteNodesCommand cmd({added->id});
  ASSERT_TRUE(cmd.execute(graph));
  ASSERT_EQ(graph.nodes.size(), 0);

  ASSERT_TRUE(cmd.undo(graph));
  ASSERT_EQ(graph.nodes.size(), 1);

  auto *restored = dynamic_cast<ConstantFloatNode *>(graph.nodes[0].get());
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->type, NodeType::Constant);
  EXPECT_EQ(restored->name, "DeleteMe");
  EXPECT_FLOAT_EQ(restored->gui_x, 5.f);
  EXPECT_FLOAT_EQ(restored->gui_y, 6.f);
  EXPECT_FLOAT_EQ(restored->value, 2.5f);
}

TEST_F(NodeSerializationTest, AddNodeCommandUndoRedoRemovesNode) {
  NodeGraph graph;
  auto node = ConstantFloatNode::create(1.11f);
  node->name = "UndoMe";

  AddNodeCommand cmd(std::move(node));

  ASSERT_TRUE(cmd.execute(graph));
  ASSERT_EQ(graph.nodes.size(), 1);

  ASSERT_TRUE(cmd.undo(graph)); // BEFORE fix: false (id 0 not found)
  ASSERT_EQ(graph.nodes.size(), 0);

  ASSERT_TRUE(cmd.execute(graph)); // redo
  ASSERT_EQ(graph.nodes.size(), 1);

  ASSERT_TRUE(cmd.undo(graph)); // undo of the redo must also work
  ASSERT_EQ(graph.nodes.size(), 0);
}
