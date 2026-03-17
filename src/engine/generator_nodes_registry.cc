#include "../include/engine/nodes/generator_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_generator_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<ConstantFloatNode>(
      NodeType::Constant,
      "Constant",
      "Generators",
      "Outputs a constant value"
    );

    registry.register_node<TimeNode>(
      NodeType::Time,
      "Time",
      "Generators",
      "Outputs the global time counter"
    );

    registry.register_node<NoiseNode>(
      NodeType::Noise,
      "Noise",
      "Generators",
      "Outputs a random noise value"
    );

    registry.register_node<StepSequencerNode>(
      NodeType::StepSequencer,
      "Step",
      "Generators",
      "Outputs a sequence of values based on a trigger input"
    );

    registry.register_node<RandomNode>(
      NodeType::Random,
      "Random",
      "Generators",
      "Outputs a random value within a specified range"
    );
  }
} // namespace node_registration
