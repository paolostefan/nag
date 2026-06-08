#include "../include/engine/nodes/generator_nodes.h"
#include "engine/nodes/particle_emitter_node.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_generator_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<ConstantFloatNode>(
      NodeType::Constant,
      "Generators",
      "Outputs a constant value"
    );

    registry.register_node<TimeNode>(
      NodeType::Time,
      "Generators",
      "Outputs the global time counter"
    );

    registry.register_node<NoiseNode>(
      NodeType::Noise,
      "Generators",
      "Outputs a random noise value"
    );

    registry.register_node<StepSequencerNode>(
      NodeType::StepSequencer,
      "Generators",
      "Outputs a sequence of values based on a trigger input"
    );

    registry.register_node<RandomNode>(
      NodeType::Random,
      "Generators",
      "Outputs a random value within a specified range"
    );

    registry.register_node<ParticleEmitterNode>(
      NodeType::ParticleEmitter,
      "Generators",
      "Emits 2D particles at a given rate with configurable speed and lifetime"
    );
  }
} // namespace node_registration
