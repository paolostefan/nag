#ifndef NAG_NODE_GRAPH_H
#define NAG_NODE_GRAPH_H

#include <memory>
#include <vector>

#include "engine/id_generator.h"
#include "engine/node.h"
#include "engine/stream.h"

struct Link {
  uint64_t id{};
  uint64_t start_pin_id{};
  uint64_t end_pin_id{};
};

struct NodeGraph {
  uint64_t id;

  IdGenerator node_id_generator;
  IdGenerator pin_id_generator;
  IdGenerator link_id_generator;

  std::string name{"<unnamed>"};
  std::vector<std::unique_ptr<Node> > nodes;
  std::vector<Link> links;

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

  void add_link(const Pin &start_pin, const Pin &end_pin);

  void add_link(const Pin *start_pin, const Pin *end_pin) { add_link(*start_pin, *end_pin); }

  void add_link(uint64_t start_pin_id, uint64_t end_pin_id);
};


#endif //NAG_NODE_GRAPH_H
