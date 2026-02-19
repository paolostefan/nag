#include "engine/node_graph.h"

Node *NodeGraph::add_node(std::unique_ptr<Node> &&node) {
  node->id = node_id_generator.generate_id();

  for (auto &pin: node->inputs) {
    pin.id = pin_id_generator.generate_id();
  }

  for (auto &pin: node->outputs) {
    pin.id = pin_id_generator.generate_id();

    // Create a stream for each output pin
    if (!pin.stream) {
      pin.stream = std::make_shared<Stream<float> >();
    }
  }

  nodes.emplace_back(std::move(node));
  return nodes.back().get();
}

bool NodeGraph::add_link(const Pin &start_pin, const Pin &end_pin) {
  // Validate pin directions
  if (start_pin.direction != Output || end_pin.direction != Input) {
    spdlog::error("Invalid pin direction(s) for link");
    return false;
  }

  // Validate pin streams
  if (!start_pin.stream) {
    spdlog::error("Invalid start pin stream for link");
    return false;
  }

  // Find the actual pins in the graph
  const Pin *actual_start_pin = nullptr;
  Pin *actual_end_pin = nullptr;

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
    spdlog::error("Cannot find actual pins in the graph");
    return false;
  }

  // Connect pins
  actual_end_pin->stream = actual_start_pin->stream;

  Link link;
  link.id = link_id_generator.generate_id();
  link.start_pin_id = actual_start_pin->id;
  link.end_pin_id = actual_end_pin->id;
  links.push_back(link);

  return true;
}

bool NodeGraph::add_link(const uint32_t start_pin_id, const uint32_t end_pin_id) {
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
    return add_link(*start_pin, *end_pin);
  }

  spdlog::error("Invalid link");
  return false;
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
