#include "gtest/gtest.h"

#include "engine/node.h"
#include "engine/generator_nodes.h"
#include "engine/math_nodes.h"
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

  ASSERT_EQ(a->outputs[0].stream, nullptr);
  ASSERT_EQ(b->outputs[0].stream, nullptr);

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

  EXPECT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream)->value, 6.0f);
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

  ASSERT_EQ(a->outputs[0].stream, nullptr);
  ASSERT_EQ(b->outputs[0].stream, nullptr);

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

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream)->value,
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

  EXPECT_FLOAT_EQ(dynamic_cast<Stream<float> *>(node_ptr->outputs[0].stream)->value, 50.0f);
}