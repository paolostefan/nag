#include "engine/math_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_math_nodes() {
    // MultiplyNode
    NodeRegistry::instance().register_node<MultiplyNode>(
      NodeType::Multiply,
      "Multiply",
      "Math",
      "Multiplies two inputs"
    );
    // DivideNode
    NodeRegistry::instance().register_node<DivideNode>(
      NodeType::Divide,
      "Divide",
      "Math",
      "Divides two inputs"
    );
    NodeRegistry::instance().register_node<MaxNode>(
      NodeType::Max,
      "Max",
      "Math",
      "Selects the larger input"
    );
    NodeRegistry::instance().register_node<MinNode>(
      NodeType::Min,
      "Min",
      "Math",
      "Selects the smaller input"
    );
    // AddNode
    NodeRegistry::instance().register_node<AddNode>(
      NodeType::Add,
      "Add",
      "Math",
      "Adds two or more inputs"
    );
    NodeRegistry::instance().register_node<SubtractNode>(
      NodeType::Subtract,
      "Subtract",
      "Math",
      "Subtracts two inputs"
    );
    NodeRegistry::instance().register_node<ModuloNode>(
      NodeType::Modulo,
      "Modulo",
      "Math",
      "Calculates the modulo of two inputs"
    );
    NodeRegistry::instance().register_node<PowerNode>(
      NodeType::Power,
      "Power",
      "Math",
      "Raises the first input to the power of the second input"
    );
    NodeRegistry::instance().register_node<AbsNode>(
      NodeType::Abs,
      "Abs",
      "Math",
      "Calculates the absolute value of the input"
    );
    NodeRegistry::instance().register_node<FloorNode>(
      NodeType::Floor,
      "Floor",
      "Math",
      "Rounds the input down to the nearest integer"
    );
    NodeRegistry::instance().register_node<CeilNode>(
      NodeType::Ceil,
      "Ceil",
      "Math",
      "Rounds the input up to the nearest integer"
    );
  }
} // namespace node_registration
