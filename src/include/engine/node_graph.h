#ifndef NAG_NODE_GRAPH_H
#define NAG_NODE_GRAPH_H

#include <memory>
#include <vector>

#include "engine/id_generator.h"
#include "engine/node.h"
#include "engine/stream.h"


struct NodeGraph {
  uint64_t id;

  IdGenerator node_id_generator;
  IdGenerator pin_id_generator;

  std::string name{"<unnamed>"};
  std::vector<std::unique_ptr<Node> > nodes;
  std::vector<std::unique_ptr<StreamBase> > streams;

  void evaluate() const;

  void add_node(std::unique_ptr<Node> &&node) {
    node->id = node_id_generator.generate_id();

    for (auto &pin: node->inputs) {
      pin.id = pin_id_generator.generate_id();
    }

    for (auto &pin: node->outputs) {
      pin.id = pin_id_generator.generate_id();
    }

    nodes.emplace_back(std::move(node));
  }

  void add_stream(std::unique_ptr<StreamBase> &&stream) {
    streams.emplace_back(std::move(stream));
  }
};


#endif //NAG_NODE_GRAPH_H
