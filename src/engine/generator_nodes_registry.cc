#include "../include/engine/nodes/generator_nodes.h"
#include "engine/nodes/particle_emitter_node.h"
#include "engine/nodes/particle_system_node.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_generator_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node(NodeType::Constant, "Generators", "Outputs a constant value",
                           [] { return ConstantFloatNode::create(); });

    registry.register_node(NodeType::Time, "Generators", "Outputs the global time counter",
                           [] { return TimeNode::create(); });

    registry.register_node(NodeType::Noise, "Generators", "Outputs a random noise value",
                           [] { return NoiseNode::create(); });

    registry.register_node(NodeType::StepSequencer, "Generators",
                           "Outputs a sequence of values based on a trigger input",
                           [] { return StepSequencerNode::create(); });

    registry.register_node(NodeType::Random, "Generators", "Outputs a random value within a specified range",
                           [] { return RandomNode::create(); });

    registry.register_node(NodeType::ParticleEmitter, "Generators",
                           "Emits 2D particles at a given rate with configurable speed and lifetime",
                           [] { return ParticleEmitterNode::create(); });

    registry.register_node(NodeType::ParticleSystem, "Generators",
                           "Manages a system of 2D particles with configurable behavior",
                           [] { return ParticleSystemNode::create(); });
  }
} // namespace node_registration
