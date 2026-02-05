#include "engine/node_graph.h"

Node *NodeGraph::add_node(std::unique_ptr<Node> &&node) {
  node->id = node_id_generator.generate_id();

  for (auto &pin: node->inputs) {
    pin.id = pin_id_generator.generate_id();
  }

  for (auto &pin: node->outputs) {
    pin.id = pin_id_generator.generate_id();

    // Create a stream for each output pin
    auto stream = std::make_unique<Stream<float> >();
    pin.stream = stream.get();
    streams.push_back(std::move(stream));
  }

  nodes.emplace_back(std::move(node));
  return nodes.back().get();
}

void NodeGraph::add_link(const Pin &start_pin, const Pin &end_pin) {

  // Validate pin directions
  if (start_pin.direction != Output || end_pin.direction != Input) {
    throw std::runtime_error("Invalid pin direction(s) for link");
  }

  // Validate pin streams
  if (!start_pin.stream) {
    throw std::runtime_error("Invalid start pin stream for link");
  }

  // Find the actual pins in the graph
  Pin *actual_end_pin = nullptr;
  Pin *actual_start_pin = nullptr;

  for (const auto &node: nodes) {
    for (auto &pin: node->inputs) {
      if (pin.id == end_pin.id) {
        actual_end_pin = &pin;
      }
    }

    for (auto &pin: node->outputs) {
      if (pin.id == start_pin.id) {
        actual_start_pin = &pin;
      }
    }
  }

  if (!actual_start_pin || !actual_end_pin) {
    throw std::runtime_error("Cannot find actual pins in the graph");
  }

  // Connect pins
  actual_end_pin->stream = actual_start_pin->stream;

  Link link;
  link.id = link_id_generator.generate_id();
  link.start_pin_id = actual_start_pin->id;
  link.end_pin_id = actual_end_pin->id;
  links.push_back(link);
}

void NodeGraph::add_link(const uint64_t start_pin_id, const uint64_t end_pin_id ) {
  // Find input and output pin
  const Pin *start_pin = nullptr;
  const Pin *end_pin = nullptr;

  for (const auto &node: nodes) {
    for (auto &pin: node->outputs) {
      if (pin.id == start_pin_id) {
        start_pin = &pin;
      }
    }

    for (auto &pin: node->inputs) {
      if (pin.id == end_pin_id) {
        end_pin = &pin;
      }
    }
  }

  if (start_pin && end_pin) {
    add_link(*start_pin, *end_pin);
  } else {
    throw std::runtime_error("Invalid link");
  }
}

void NodeGraph::evaluate() const {
  bool progress;
  do {
    progress = false;

    for (const auto &node: nodes) {
      if (node->needs_evaluation()) {
        node->evaluate();
        progress = true;
      }
    }
  } while (progress);
}
