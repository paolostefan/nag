#include "../include/engine/nodes/math_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_math_nodes() {
    // MultiplyNode
    NodeRegistry::instance().register_node<MultiplyNode>(
      NodeType::Multiply,
      "Math",
      "Multiplies two inputs"
    );
    // DivideNode
    NodeRegistry::instance().register_node<DivideNode>(
      NodeType::Divide,
      "Math",
      "Divides two inputs"
    );
    NodeRegistry::instance().register_node<MaxNode>(
      NodeType::Max,
      "Math",
      "Selects the larger input"
    );
    NodeRegistry::instance().register_node<MinNode>(
      NodeType::Min,
      "Math",
      "Selects the smaller input"
    );
    // AddNode
    NodeRegistry::instance().register_node<AddNode>(
      NodeType::Add,
      "Math",
      "Adds two or more inputs"
    );
    NodeRegistry::instance().register_node<SubtractNode>(
      NodeType::Subtract,
      "Math",
      "Subtracts two inputs"
    );
    NodeRegistry::instance().register_node<ModuloNode>(
      NodeType::Modulo,
      "Math",
      "Calculates the modulo of two inputs"
    );
    NodeRegistry::instance().register_node<PowerNode>(
      NodeType::Power,
      "Math",
      "Raises the first input to the power of the second input"
    );
    NodeRegistry::instance().register_node<AbsNode>(
      NodeType::Abs,
      "Math",
      "Calculates the absolute value of the input"
    );
    NodeRegistry::instance().register_node<FloorNode>(
      NodeType::Floor,
      "Math",
      "Rounds the input down to the nearest integer"
    );
    NodeRegistry::instance().register_node<CeilNode>(
      NodeType::Ceil,
      "Math",
      "Rounds the input up to the nearest integer"
    );
    NodeRegistry::instance().register_node<RoundNode>(
      NodeType::Round,
      "Math",
      "Rounds the input to the nearest integer"
    );
    NodeRegistry::instance().register_node<SqrtNode>(
      NodeType::Sqrt,
      "Math",
      "Computes the square root of the input"
    );
    NodeRegistry::instance().register_node<NegateNode>(
      NodeType::Negate,
      "Math",
      "Negates the input value"
    );
    NodeRegistry::instance().register_node<SinNode>(
      NodeType::Sin,
      "Math",
      "Computes the sine of the input"
    );
    NodeRegistry::instance().register_node<CosNode>(
      NodeType::Cos,
      "Math",
      "Computes the cosine of the input"
    );
    NodeRegistry::instance().register_node<TanNode>(
      NodeType::Tan,
      "Math",
      "Computes the tangent of the input"
    );
    NodeRegistry::instance().register_node<RemapNode>(
      NodeType::Remap,
      "Math",
      "Remaps input from one range to another"
    );
    NodeRegistry::instance().register_node<ClampNode>(
      NodeType::Clamp,
      "Math",
      "Clamps input to a specified range"
    );
    NodeRegistry::instance().register_node<LerpNode>(
      NodeType::Lerp,
      "Math",
      "Linear interpolation between two values"
    );
    NodeRegistry::instance().register_node<SmoothStepNode>(
      NodeType::SmoothStep,
      "Math",
      "Smooth interpolation with ease in/out"
    );
  }
} // namespace node_registration
