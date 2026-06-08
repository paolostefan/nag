#include "../include/engine/nodes/temporal_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_temporal_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node<LFONode>(
      NodeType::LFO,
      "Temporal",
      "Low frequency oscillator with multiple waveforms"
    );

    registry.register_node<EnvelopeNode>(
      NodeType::Envelope,
      "Temporal",
      "ADSR envelope generator"
    );

    registry.register_node<DelayNode>(
      NodeType::Delay,
      "Temporal",
      "Time-based signal delay with circular buffer"
    );

    registry.register_node<SmootherNode>(
      NodeType::Smoother,
      "Temporal",
      "Exponential smoothing filter (one-pole lowpass)"
    );
  }
} // namespace node_registration
