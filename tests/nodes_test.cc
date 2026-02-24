#include "gtest/gtest.h"

#include "engine/node.h"
#include "engine/generator_nodes.h"
#include "engine/math_nodes.h"
#include "engine/temporal_nodes.h"
#include "engine/node_graph.h"

class NodesTest : public testing::Test {
protected:
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
    node_graph = new NodeGraph{};
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
    delete node_graph;
  }

  NodeGraph *node_graph{nullptr};
};


TEST_F(NodesTest, MultiplyNode) {
  auto node = MultiplyNode::create();
  EXPECT_EQ(node->type, Multiply);
  EXPECT_EQ(node->name, "Multiply");
  EXPECT_EQ(node->inputs.size(), 2);
  EXPECT_EQ(node->outputs.size(), 1);

  auto a = ConstantFloatNode::create(2.0f);
  auto b = ConstantFloatNode::create(3.0f);

  ASSERT_EQ(a->outputs.size(), 1);
  ASSERT_EQ(b->outputs.size(), 1);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);

  // Force evaluation of constant nodes
  a_ptr->evaluate();
  b_ptr->evaluate();

  // Evaluation of multiply node should be triggered by the constant nodes
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 6.0f);
}

TEST_F(NodesTest, DivideNodeDivideByZero) {
  auto node = DivideNode::create();
  EXPECT_EQ(node->type, Divide);
  EXPECT_EQ(node->name, "Divide");
  EXPECT_EQ(node->inputs.size(), 2);
  EXPECT_EQ(node->outputs.size(), 1);

  auto a = ConstantFloatNode::create(2.0f);
  auto b = ConstantFloatNode::create(0.0f);

  ASSERT_EQ(a->outputs.size(), 1);
  ASSERT_EQ(b->outputs.size(), 1);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);

  // Force evaluation of constant nodes
  a_ptr->evaluate();
  b_ptr->evaluate();

  // Evaluation of divide node should be triggered by the constant nodes
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value,
                  2.0f/DivideNode::kEpsilon);
}

TEST_F(NodesTest, RemapNode)
{
  auto node = RemapNode::create(0,1,0,100);
  EXPECT_EQ(node->type, Remap);
  EXPECT_EQ(node->name, "Remap");

  auto const_node = ConstantFloatNode::create(0.5f);
  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto const_ptr = node_graph->add_node(std::move(const_node));

  node_graph->add_link(const_ptr->outputs[0], node_ptr->inputs[0]);

  // Force evaluation of constant node to start
  const_ptr->evaluate();

  // Evaluation of remap node should be triggered by the constant node
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 50.0f);
}

TEST_F(NodesTest, LFONodeTest) {
  auto node = LFONode::create(
    .5f,
    .5f,
    LFONode::WaveShape::Sine,
    0.f,
    .5f);
  EXPECT_EQ(node->type, LFO);
  EXPECT_EQ(node->name, "LFO");
  EXPECT_EQ(node->inputs.size(), 1);
  EXPECT_EQ(node->outputs.size(), 1);

  auto time_node = TimeNode::create();
  const auto time_node_ptr = node_graph->add_node(std::move(time_node));
  const auto lfo_node = node_graph->add_node(std::move(node));
  node_graph->add_link(time_node_ptr->outputs[0], lfo_node->inputs[0]);

  // Force evaluation of time node to start
  time_node_ptr->evaluate();

  // Trigger evaluation
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(lfo_node->outputs[0].stream.get())->value, 0.5f);
}

// ===========================================================================
// GENERATOR NODE TESTS
// ===========================================================================

TEST_F(NodesTest, ConstantFloatNode) {
  auto node = ConstantFloatNode::create(42.0f);
  EXPECT_EQ(node->type, Constant);
  EXPECT_EQ(node->name, "Constant");
  EXPECT_EQ(node->inputs.size(), 0);
  EXPECT_EQ(node->outputs.size(), 1);

  const auto node_ptr = node_graph->add_node(std::move(node));
  node_ptr->evaluate();

  EXPECT_FLOAT_EQ(
    dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 42.0f);
}

TEST_F(NodesTest, TimeNode) {
  auto node = TimeNode::create();
  EXPECT_EQ(node->type, Time);
  EXPECT_EQ(node->name, "Time");
  EXPECT_EQ(node->inputs.size(), 0);
  EXPECT_EQ(node->outputs.size(), 1);

  const auto node_ptr = node_graph->add_node(std::move(node));

  // Initial time should be 0
  node_ptr->evaluate();
  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 0.0f);

  // Step forward by 0.5 seconds
  dynamic_cast<TimeNode*>(node_ptr)->step(0.5f);
  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 0.5f);
}

TEST_F(NodesTest, NoiseNode) {
  auto node = NoiseNode::create(1.0f, 1.0f, 2, 0.5f);
  EXPECT_EQ(node->type, Noise);
  EXPECT_EQ(node->name, "Noise");
  EXPECT_EQ(node->inputs.size(), 1);
  EXPECT_EQ(node->outputs.size(), 1);

  auto input_node = ConstantFloatNode::create(1.0f);
  const auto input_ptr = node_graph->add_node(std::move(input_node));
  const auto noise_ptr = node_graph->add_node(std::move(node));

  node_graph->add_link(input_ptr->outputs[0], noise_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  // Just verify it produces a value (noise is deterministic based on input)
  const float result = dynamic_cast<Stream<float> *>(noise_ptr->outputs[0].stream.get())->value;
  EXPECT_TRUE(std::isfinite(result));
}

TEST_F(NodesTest, RandomNode) {
  auto node = std::make_unique<RandomNode>();
  node->set_range(0.0f, 10.0f);
  node->set_seed(42);
  EXPECT_EQ(node->name, "Random");
  EXPECT_EQ(node->type, Random);

  auto trigger_node = ConstantFloatNode::create(1.0f);

  node->add_input("trigger");
  node->add_output("random");

  const auto trigger_ptr = node_graph->add_node(std::move(trigger_node));
  const auto random_ptr = node_graph->add_node(std::move(node));

  node_graph->add_link(trigger_ptr->outputs[0], random_ptr->inputs[0]);

  trigger_ptr->evaluate();
  node_graph->evaluate();

  const float result = dynamic_cast<Stream<float> *>(random_ptr->outputs[0].stream.get())->value;
  EXPECT_GE(result, 0.0f);
  EXPECT_LE(result, 10.0f);
}

TEST_F(NodesTest, StepSequencerNode) {
  auto node = std::make_unique<StepSequencerNode>();
  node->set_steps({1.0f, 2.0f, 3.0f, 4.0f});
  EXPECT_EQ(node->name, "StepSequencer");
  EXPECT_EQ(node->type, StepSequencer);

  auto trigger_node = ConstantFloatNode::create(1.0f);

  node->add_input("trigger");
  node->add_output("step");

  const auto trigger_ptr = node_graph->add_node(std::move(trigger_node));
  const auto seq_ptr = node_graph->add_node(std::move(node));

  node_graph->add_link(trigger_ptr->outputs[0], seq_ptr->inputs[0]);

  trigger_ptr->evaluate();
  node_graph->evaluate();

  // First step should be 2.0f (advances on first trigger)
  const float result = dynamic_cast<Stream<float> *>(seq_ptr->outputs[0].stream.get())->value;
  EXPECT_FLOAT_EQ(result, 2.0f);
}

// ===========================================================================
// MATH NODE TESTS - Binary Operators
// ===========================================================================

TEST_F(NodesTest, AddNode) {
  auto node = AddNode::create(3);
  EXPECT_EQ(node->type, Add);
  EXPECT_EQ(node->name, "Add");
  EXPECT_EQ(node->inputs.size(), 3);
  EXPECT_EQ(node->outputs.size(), 1);

  auto a = ConstantFloatNode::create(2.0f);
  auto b = ConstantFloatNode::create(3.0f);
  auto c = ConstantFloatNode::create(5.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));
  const auto c_ptr = node_graph->add_node(std::move(c));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);
  node_graph->add_link(c_ptr->outputs[0], node_ptr->inputs[2]);

  a_ptr->evaluate();
  b_ptr->evaluate();
  c_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 10.0f);
}

TEST_F(NodesTest, SubtractNode) {
  auto node = std::make_unique<SubtractNode>();
  node->add_input("a");
  node->add_input("b");
  node->add_output("difference");

  EXPECT_EQ(node->type, Subtract);
  EXPECT_EQ(node->name, "Subtract");

  auto a = ConstantFloatNode::create(10.0f);
  auto b = ConstantFloatNode::create(3.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);

  a_ptr->evaluate();
  b_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 7.0f);
}

TEST_F(NodesTest, ModuloNode) {
  auto node = std::make_unique<ModuloNode>();
  node->add_input("a");
  node->add_input("b");
  node->add_output("remainder");

  EXPECT_EQ(node->type, Modulo);
  EXPECT_EQ(node->name, "Modulo");

  auto a = ConstantFloatNode::create(7.5f);
  auto b = ConstantFloatNode::create(2.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);

  a_ptr->evaluate();
  b_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 1.5f);
}

TEST_F(NodesTest, PowerNode) {
  auto node = std::make_unique<PowerNode>();
  node->add_input("base");
  node->add_input("exponent");
  node->add_output("result");

  EXPECT_EQ(node->type, Power);
  EXPECT_EQ(node->name, "Power");

  auto base = ConstantFloatNode::create(2.0f);
  auto exponent = ConstantFloatNode::create(3.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto base_ptr = node_graph->add_node(std::move(base));
  const auto exp_ptr = node_graph->add_node(std::move(exponent));

  node_graph->add_link(base_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(exp_ptr->outputs[0], node_ptr->inputs[1]);

  base_ptr->evaluate();
  exp_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 8.0f);
}

TEST_F(NodesTest, MinNode) {
  auto node = MinNode::create(2);
  node->add_output("min");

  EXPECT_EQ(node->type, Min);
  EXPECT_EQ(node->name, "Min");

  auto a = ConstantFloatNode::create(5.0f);
  auto b = ConstantFloatNode::create(3.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);

  a_ptr->evaluate();
  b_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 3.0f);
}

TEST_F(NodesTest, MaxNode) {
  auto node = MaxNode::create(2);
  node->add_output("max");

  EXPECT_EQ(node->type, Max);
  EXPECT_EQ(node->name, "Max");

  auto a = ConstantFloatNode::create(5.0f);
  auto b = ConstantFloatNode::create(3.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);

  a_ptr->evaluate();
  b_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 5.0f);
}

// ===========================================================================
// MATH NODE TESTS - Unary Operators
// ===========================================================================

TEST_F(NodesTest, AbsNode) {
  auto node = std::make_unique<AbsNode>();
  node->add_input("in");
  node->add_output("out");

  EXPECT_EQ(node->type, Abs);
  EXPECT_EQ(node->name, "Abs");

  auto input = ConstantFloatNode::create(-5.5f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 5.5f);
}

TEST_F(NodesTest, FloorNode) {
  auto node = std::make_unique<FloorNode>();
  node->add_input("in");
  node->add_output("out");

  EXPECT_EQ(node->type, Floor);
  EXPECT_EQ(node->name, "Floor");

  auto input = ConstantFloatNode::create(3.7f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 3.0f);
}

TEST_F(NodesTest, CeilNode) {
  auto node = std::make_unique<CeilNode>();
  node->add_input("in");
  node->add_output("out");

  EXPECT_EQ(node->type, Ceil);
  EXPECT_EQ(node->name, "Ceil");

  auto input = ConstantFloatNode::create(3.2f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 4.0f);
}

TEST_F(NodesTest, RoundNode) {
  auto node = std::make_unique<RoundNode>();
  node->add_input("in");
  node->add_output("out");

  EXPECT_EQ(node->type, Round);
  EXPECT_EQ(node->name, "Round");

  auto input = ConstantFloatNode::create(3.6f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 4.0f);
}

TEST_F(NodesTest, SqrtNode) {
  auto node = std::make_unique<SqrtNode>();
  node->add_input("in");
  node->add_output("out");

  EXPECT_EQ(node->type, Sqrt);
  EXPECT_EQ(node->name, "Sqrt");

  auto input = ConstantFloatNode::create(16.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 4.0f);
}

TEST_F(NodesTest, SqrtNodeNegativeInput) {
  auto node = std::make_unique<SqrtNode>();
  node->add_input("in");
  node->add_output("out");

  auto input = ConstantFloatNode::create(-4.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  // Should clamp to 0 and return sqrt(0) = 0
  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 0.0f);
}

TEST_F(NodesTest, NegateNode) {
  auto node = std::make_unique<NegateNode>();
  node->add_input("in");
  node->add_output("out");

  EXPECT_EQ(node->type, Negate);
  EXPECT_EQ(node->name, "Negate");

  auto input = ConstantFloatNode::create(5.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, -5.0f);
}

TEST_F(NodesTest, SinNode) {
  auto node = SinNode::create();
  EXPECT_EQ(node->type, Sin);
  EXPECT_EQ(node->name, "Sin");
  EXPECT_EQ(node->inputs.size(), 1);
  EXPECT_EQ(node->outputs.size(), 1);

  auto input = ConstantFloatNode::create(0.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 0.0f);
}

TEST_F(NodesTest, CosNode) {
  auto node = CosNode::create();
  EXPECT_EQ(node->type, Cos);
  EXPECT_EQ(node->name, "Cos");
  EXPECT_EQ(node->inputs.size(), 1);
  EXPECT_EQ(node->outputs.size(), 1);

  auto input = ConstantFloatNode::create(0.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 1.0f);
}

TEST_F(NodesTest, TanNode) {
  auto node = TanNode::create();
  EXPECT_EQ(node->type, Tan);
  EXPECT_EQ(node->name, "Tan");
  EXPECT_EQ(node->inputs.size(), 1);
  EXPECT_EQ(node->outputs.size(), 1);

  auto input = ConstantFloatNode::create(0.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 0.0f);
}

// ===========================================================================
// MATH NODE TESTS - Special Operators
// ===========================================================================

TEST_F(NodesTest, ClampNode) {
  auto node = ClampNode::create(0.0f, 10.0f);
  EXPECT_EQ(node->type, Clamp);
  EXPECT_EQ(node->name, "Clamp");
  EXPECT_EQ(node->inputs.size(), 1);
  EXPECT_EQ(node->outputs.size(), 1);

  auto input = ConstantFloatNode::create(15.0f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 10.0f);
}

TEST_F(NodesTest, LerpNode) {
  auto node = LerpNode::create();
  EXPECT_EQ(node->type, Lerp);
  EXPECT_EQ(node->name, "Lerp");
  EXPECT_EQ(node->inputs.size(), 3);
  EXPECT_EQ(node->outputs.size(), 1);

  auto a = ConstantFloatNode::create(0.0f);
  auto b = ConstantFloatNode::create(100.0f);
  auto t = ConstantFloatNode::create(0.25f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto a_ptr = node_graph->add_node(std::move(a));
  const auto b_ptr = node_graph->add_node(std::move(b));
  const auto t_ptr = node_graph->add_node(std::move(t));

  node_graph->add_link(a_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(b_ptr->outputs[0], node_ptr->inputs[1]);
  node_graph->add_link(t_ptr->outputs[0], node_ptr->inputs[2]);

  a_ptr->evaluate();
  b_ptr->evaluate();
  t_ptr->evaluate();
  node_graph->evaluate();

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 25.0f);
}

TEST_F(NodesTest, SmoothStepNode) {
  auto node = SmoothStepNode::create(0.0f, 1.0f);
  EXPECT_EQ(node->type, SmoothStep);
  EXPECT_EQ(node->name, "SmoothStep");
  EXPECT_EQ(node->inputs.size(), 1);
  EXPECT_EQ(node->outputs.size(), 1);

  auto input = ConstantFloatNode::create(0.5f);

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto input_ptr = node_graph->add_node(std::move(input));

  node_graph->add_link(input_ptr->outputs[0], node_ptr->inputs[0]);

  input_ptr->evaluate();
  node_graph->evaluate();

  // At t=0.5, smoothstep should give 0.5
  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 0.5f);
}

// ===========================================================================
// TEMPORAL NODE TESTS
// ===========================================================================

TEST_F(NodesTest, EnvelopeNode) {
  auto node = EnvelopeNode::create(0.1f, 0.1f, 0.7f, 0.2f);
  EXPECT_EQ(node->type, Envelope);
  EXPECT_EQ(node->name, "Envelope");
  EXPECT_EQ(node->inputs.size(), 2);
  EXPECT_EQ(node->outputs.size(), 1);

  auto trigger = ConstantFloatNode::create(0.0f);
  auto time_node = TimeNode::create();

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto trigger_ptr = node_graph->add_node(std::move(trigger));
  const auto time_ptr = node_graph->add_node(std::move(time_node));

  node_graph->add_link(trigger_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(time_ptr->outputs[0], node_ptr->inputs[1]);

  trigger_ptr->evaluate();
  time_ptr->evaluate();
  node_graph->evaluate();

  // Initially should be idle (0.0)
  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 0.0f);
}

TEST_F(NodesTest, DelayNode) {
  auto node = DelayNode::create(0.5f, 60.0f);
  EXPECT_EQ(node->type, Delay);
  EXPECT_EQ(node->name, "Delay");
  EXPECT_EQ(node->inputs.size(), 2);
  EXPECT_EQ(node->outputs.size(), 1);

  auto value = ConstantFloatNode::create(5.0f);
  auto time_node = TimeNode::create();

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto value_ptr = node_graph->add_node(std::move(value));
  const auto time_ptr = node_graph->add_node(std::move(time_node));

  node_graph->add_link(value_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(time_ptr->outputs[0], node_ptr->inputs[1]);

  value_ptr->evaluate();
  time_ptr->evaluate();
  node_graph->evaluate();

  // Initially should output 0 (buffer is empty/initialized)
  const float result = dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value;
  EXPECT_TRUE(std::isfinite(result));
}

TEST_F(NodesTest, SmootherNode) {
  auto node = SmootherNode::create(0.1f);
  EXPECT_EQ(node->type, Smoother);
  EXPECT_EQ(node->name, "Smoother");
  EXPECT_EQ(node->inputs.size(), 2);
  EXPECT_EQ(node->outputs.size(), 1);

  auto target = ConstantFloatNode::create(10.0f);
  auto time_node = TimeNode::create();

  const auto node_ptr = node_graph->add_node(std::move(node));
  const auto target_ptr = node_graph->add_node(std::move(target));
  const auto time_ptr = node_graph->add_node(std::move(time_node));

  node_graph->add_link(target_ptr->outputs[0], node_ptr->inputs[0]);
  node_graph->add_link(time_ptr->outputs[0], node_ptr->inputs[1]);

  target_ptr->evaluate();
  time_ptr->evaluate();
  node_graph->evaluate();

  // On first evaluation, should initialize to target value
  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream.get())->value, 10.0f);
}