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
  std::vector<std::unique_ptr<StreamBase> > streams;
  std::vector<Link> links;

  void evaluate() const;

  Node *add_node(std::unique_ptr<Node> &&node);

  void add_stream(std::unique_ptr<StreamBase> &&stream) {
    streams.emplace_back(std::move(stream));
  }

  void add_link(const Pin &start_pin, const Pin &end_pin);

  void add_link(uint64_t start_pin_id, uint64_t end_pin_id);

  /**
   * Create an external stream not connected to any node.
   * Useful for streams like time_stream that are updated externally.
   *
   * @tparam T Type of the stream value
   * @param initial_value Initial value for the stream
   * @return Pointer to the created stream
   */
  template<typename T>
  Stream<T> *create_external_stream(const T &initial_value = T{}) {
    auto stream = std::make_unique<Stream<T> >(initial_value);
    Stream<T> *ptr = stream.get();
    streams.push_back(std::move(stream));
    return ptr;
  }
};


#endif //NAG_NODE_GRAPH_H
