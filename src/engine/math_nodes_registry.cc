#include "../include/engine/nodes/math_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_math_nodes() {
    // AddNode
    NodeRegistry::instance().register_node(NodeType::Add, "Math", "Adds two or more inputs", [] { return AddNode::create(); });
    // SubtractNode
    NodeRegistry::instance().register_node(NodeType::Subtract, "Math", "Subtracts two inputs", [] { return SubtractNode::create(); });
    // MultiplyNode
    NodeRegistry::instance().register_node(NodeType::Multiply, "Math", "Multiplies two inputs", [] { return MultiplyNode::create(); });
    // DivideNode
    NodeRegistry::instance().register_node(NodeType::Divide, "Math", "Divides two inputs", [] { return DivideNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Max, "Math", "Selects the larger input", [] { return MaxNode::create(2); });
    // ModuloNode
    NodeRegistry::instance().register_node(NodeType::Modulo, "Math", "Calculates the modulo of two inputs", [] { return ModuloNode::create(); });
    // PowerNode
    NodeRegistry::instance().register_node(NodeType::Power, "Math", "Raises the first input to the power of the second input", [] { return PowerNode::create(); });
    // MinNode
    NodeRegistry::instance().register_node(NodeType::Min, "Math", "Selects the smaller input", [] { return MinNode::create(); });
    // CompareNode
    NodeRegistry::instance().register_node(NodeType::Compare, "Math", "Compares two inputs and outputs true if the first is greater, otherwise false", [] { return CompareNode::create(); });
    // AbsNode
    NodeRegistry::instance().register_node(NodeType::Abs, "Math", "Calculates the absolute value of the input", [] { return AbsNode::create(); });
    // FloorNode
    NodeRegistry::instance().register_node(NodeType::Floor, "Math", "Rounds the input down to the nearest integer", [] { return FloorNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Ceil, "Math", "Rounds the input up to the nearest integer", [] { return CeilNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Round, "Math", "Rounds the input to the nearest integer", [] { return RoundNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Sqrt, "Math", "Computes the square root of the input", [] { return SqrtNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Negate, "Math", "Negates the input value", [] { return NegateNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Sin, "Math", "Computes the sine of the input", [] { return SinNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Cos, "Math", "Computes the cosine of the input", [] { return CosNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Tan, "Math", "Computes the tangent of the input", [] { return TanNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Remap, "Math", "Remaps input from one range to another", [] { return RemapNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Clamp, "Math", "Clamps input to a specified range", [] { return ClampNode::create(); });
    NodeRegistry::instance().register_node(NodeType::Lerp, "Math", "Linear interpolation between two values", [] { return LerpNode::create(); });
    NodeRegistry::instance().register_node(NodeType::SmoothStep, "Math", "Smooth interpolation with ease in/out", [] { return SmoothStepNode::create(0.f, 1.f); });
  }
} // namespace node_registration
