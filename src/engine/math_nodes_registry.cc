#include "engine/math_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_math_nodes() {
    // MultiplyNode
    NodeRegistry::instance().register_node<MultiplyNode>(
      Multiply,
      "Multiply",
      "Math",
      "Multiplies two inputs"
    );
    // DivideNode
    NodeRegistry::instance().register_node<DivideNode>(
      Divide,
      "Divide",
      "Math",
      "Divides two inputs"
    );
    NodeRegistry::instance().register_node<MaxNode>(
      Max,
      "Max",
      "Math",
      "Selects the larger input"
    );
    NodeRegistry::instance().register_node<MinNode>(
      Min,
      "Min",
      "Math",
      "Selects the smaller input"
    );
    // AddNode
    NodeRegistry::instance().register_node<AddNode>(
      Add,
      "Add",
      "Math",
      "Adds two or more inputs"
    );
    NodeRegistry::instance().register_node<SubtractNode>(
      Subtract,
      "Subtract",
      "Math",
      "Subtracts two inputs"
    );
    NodeRegistry::instance().register_node<ModuloNode>(
      Modulo,
      "Modulo",
      "Math",
      "Calculates the modulo of two inputs"
    );
    NodeRegistry::instance().register_node<PowerNode>(
      Power,
      "Power",
      "Math",
      "Raises the first input to the power of the second input"
    );
    NodeRegistry::instance().register_node<AbsNode>(
      Abs,
      "Abs",
      "Math",
      "Calculates the absolute value of the input"
    );
    NodeRegistry::instance().register_node<FloorNode>(
      Floor,
      "Floor",
      "Math",
      "Rounds the input down to the nearest integer"
    );
    NodeRegistry::instance().register_node<CeilNode>(
      Ceil,
      "Ceil",
      "Math",
      "Rounds the input up to the nearest integer"
    );
  }
} // namespace node_registration
