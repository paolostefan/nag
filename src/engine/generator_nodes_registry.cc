#include "../include/engine/nodes/generator_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_generator_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<ConstantFloatNode>(
      NodeType::Constant,
      "Constant",
      "Generator",
      "Outputs a constant value"
    );

    registry.register_node<TimeNode>(
      NodeType::Time,
      "Time",
      "Generator",
      "Outputs the global time counter"
    );

    registry.register_node<NoiseNode>(
      NodeType::Noise,
      "Noise",
      "Generator",
      "Outputs a random noise value"
    );

    registry.register_node<StepSequencerNode>(
      NodeType::StepSequencer,
      "Step",
      "Generator",
      "Outputs a a sequence of values based on a trigger input"
    );
  }
} // namespace node_registration
