#include "engine/node_graph.h"

#include <queue>
#include <stack>
#include <unordered_set>

Node *NodeGraph::add_node(std::unique_ptr<Node> &&node) {
  node->id = node_id_generator.generate_id();

  for (auto &pin: node->inputs) {
    pin.id = pin_id_generator.generate_id();
  }

  for (auto &pin: node->outputs) {
    pin.id = pin_id_generator.generate_id();

    // Create a stream for each output pin
    if (!pin.stream) {
      pin.stream = std::make_shared<Stream>(default_stream_value(pin.data_type));
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
  // TODO: is it really necessary to search for the pins again? Can we guarantee the caller provides valid pins from this graph?
  const Pin *actual_start_pin = nullptr;
  Pin *actual_end_pin = nullptr;

  int src_node_id{}, dst_node_id{};

  for (const auto &node: nodes) {
    for (auto &pin: node->inputs) {
      if (pin.id == end_pin.id) {
        actual_end_pin = &pin;
        dst_node_id = node->id;
      }
    }

    for (auto &pin: node->outputs) {
      if (pin.id == start_pin.id) {
        actual_start_pin = &pin;
        src_node_id = node->id;
      }
    }
  }

  if (!actual_start_pin || !actual_end_pin) {
    spdlog::error("Cannot find actual pins in the graph");
    return false;
  }

  // Cycle detection
  if (has_path(dst_node_id, src_node_id)) {
    // If dst -> src already exists, linking src -> dest would create a loop
    spdlog::warn("NodeGraph::add_link : loop detected ({} - {})", dst_node_id, src_node_id);
    return false;
  }

  // Connect pins
  actual_end_pin->stream = actual_start_pin->stream;
  actual_end_pin->connected = true;

  Link link;
  link.id = link_id_generator.generate_id();
  link.start_pin_id = actual_start_pin->id;
  link.end_pin_id = actual_end_pin->id;
  links.push_back(link);

  return true;
}

bool NodeGraph::add_link(const int start_pin_id, const int end_pin_id) {
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

void NodeGraph::remove_link(const int link_id) {
  const auto it = std::ranges::find_if(links,
    [link_id](const Link &link) { return link.id == link_id; });

  if (it == links.end()) {
    spdlog::error("Link {} not found", link_id);
    return;
  }

  const Link &link = *it;

  // Detach stream from input pin
  for (const auto &node: nodes) {
    for (auto &pin: node->inputs) {
      if (pin.id == link.end_pin_id) {
        pin.stream = nullptr;
        pin.connected = false;
        break;
      }
    }
  }

  links.erase(it);
}

void NodeGraph::evaluate() const {
  // ── Topological sort (Kahn's algorithm) ───────────────────────────────────
  // Guarantees each node is evaluated exactly once, after all its predecessors.
  // Always terminates in O(N+E), regardless of stream versioning bugs.

  // 1. Build node_id → node* map
  std::unordered_map<int, Node *> node_map;
  node_map.reserve(nodes.size());
  for (const auto &node: nodes) {
    node_map[node->id] = node.get();
  }

  // 2. Build adjacency list and in-degree map from links
  //    adj[src_id] = list of dst_ids
  std::unordered_map<int, std::vector<int> > adj;
  std::unordered_map<int, int> in_degree;

  for (const auto &node: nodes) {
    in_degree.emplace(node->id, 0);
  }

  for (const auto &link: links) {
    // Resolve src and dst node IDs from pin IDs
    int src_node_id = 0;
    int dst_node_id = 0;

    for (const auto &node: nodes) {
      for (const auto &pin: node->outputs) {
        if (pin.id == link.start_pin_id) src_node_id = node->id;
      }
      for (const auto &pin: node->inputs) {
        if (pin.id == link.end_pin_id) dst_node_id = node->id;
      }
    }

    if (src_node_id != 0 && dst_node_id != 0 && src_node_id != dst_node_id) {
      adj[src_node_id].push_back(dst_node_id);
      in_degree[dst_node_id]++;
    }
  }

  // 3. Seed the queue with all source nodes (in-degree == 0)
  std::queue<int> queue;
  for (const auto &[id_, deg]: in_degree) {
    if (deg == 0) queue.push(id_);
  }

  // 4. Process nodes in topological order
  int evaluated_count = 0;
  while (!queue.empty()) {
    const int current_id = queue.front();
    queue.pop();

    Node *node = node_map[current_id];
    node->evaluate();
    ++evaluated_count;

    // Decrement in-degree of successors; enqueue any that become ready
    if (const auto it = adj.find(current_id); it != adj.end()) {
      for (const int successor_id: it->second) {
        if (--in_degree[successor_id] == 0) {
          queue.push(successor_id);
        }
      }
    }
  }

  // 5. Sanity check: if not all nodes were evaluated, a cycle exists
  if (evaluated_count != static_cast<int>(nodes.size())) {
    spdlog::error(
      "NodeGraph::evaluate: topological sort incomplete. "
      "Evaluated {}/{} nodes. A cycle may be present in the graph.",
      evaluated_count, nodes.size()
    );
  }
}

[[nodiscard]] bool NodeGraph::has_path(const int from_node_id, const int to_node_id) const {
  // Iterative DFS: search path from -> to following nodes in reverse order.
  std::unordered_set<int> visited;
  std::stack<int> stack;
  stack.push(from_node_id);

  while (!stack.empty()) {
    const int current = stack.top();
    stack.pop();

    if (current == to_node_id) return true;
    if (visited.contains(current)) continue;
    visited.insert(current);

    // Find all nodes reachable from current
    for (const auto &link: links) {
      // Find the node that owns start_pin_id
      for (const auto &node: nodes) {
        if (node->id != current) continue;
        for (const auto &pin: node->outputs) {
          if (pin.id != link.start_pin_id) continue;
          // Find destination node
          for (const auto &dst_node: nodes) {
            for (const auto &dst_pin: dst_node->inputs) {
              if (dst_pin.id == link.end_pin_id) {
                stack.push(dst_node->id);
              }
            }
          }
        }
      }
    }
  }
  return false;
}

[[nodiscard]] Node *NodeGraph::find_node(const int node_id) const {
  for (const auto &node: nodes) {
    if (node->id == node_id) return node.get();
  }
  return nullptr;
}