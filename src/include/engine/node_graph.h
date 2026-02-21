#ifndef NAG_NODE_GRAPH_H
#define NAG_NODE_GRAPH_H

#include <memory>
#include <vector>

#include "spdlog/spdlog.h"

#include "engine/id_generator.h"
#include "engine/node.h"

struct Link {
  uint32_t id{};
  uint32_t start_pin_id{};
  uint32_t end_pin_id{};
};

struct NodeGraph {
  uint32_t id{};

  IdGenerator node_id_generator;
  IdGenerator pin_id_generator;
  IdGenerator link_id_generator;

  std::string name{"<unnamed>"};
  std::vector<std::unique_ptr<Node> > nodes;
  std::vector<Link> links;

  // Delete copy constructor and copy assignment to prevent copying
  NodeGraph(const NodeGraph&) = delete;
  NodeGraph& operator=(const NodeGraph&) = delete;
  
  // Default constructor and move operations
  NodeGraph() = default;
  NodeGraph(NodeGraph&&) = default;
  NodeGraph& operator=(NodeGraph&&) = default;

  /**
   * @brief Clears all nodes and links from the graph.
   *
   * Resets the graph to an empty state. All node and stream
   * pointers previously obtained from this graph become invalid.
   */
  void clear() noexcept {
    links.clear();
    nodes.clear();
  }

  void evaluate() const;

  Node *add_node(std::unique_ptr<Node> &&node);

  bool add_link(const Pin &start_pin, const Pin &end_pin);

  bool add_link(const Pin *start_pin, const Pin *end_pin) {
    if (!start_pin || !end_pin) {
      spdlog::error("Invalid pin(s)");
      return false;
    }
    return add_link(*start_pin, *end_pin);
  }

  bool add_link(uint32_t start_pin_id, uint32_t end_pin_id);

  [[nodiscard]] bool has_path(uint32_t from_node_id, uint32_t to_node_id) const;
};


#endif //NAG_NODE_GRAPH_H
