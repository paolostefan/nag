#include "../include/engine/nodes/temporal_nodes.h"
#include "engine/node_registry.h"

namespace node_registration {
  void register_temporal_nodes() {
    auto &registry = NodeRegistry::instance();

    registry.register_node(NodeType::LFO, "Temporal", "Low frequency oscillator with multiple waveforms", [] { return LFONode::create(); });

    registry.register_node(NodeType::Envelope, "Temporal", "ADSR envelope generator", [] { return EnvelopeNode::create(); });

    registry.register_node(NodeType::Delay, "Temporal", "Time-based signal delay with circular buffer", [] { return DelayNode::create(); });

    registry.register_node(NodeType::Smoother, "Temporal", "Exponential smoothing filter (one-pole lowpass)", [] { return SmootherNode::create(); });
  }
} // namespace node_registration
