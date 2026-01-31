#ifndef NAG_NODE_GRAPH_H
#define NAG_NODE_GRAPH_H

#include <memory>
#include <vector>

#include "engine/node.h"
#include "engine/stream.h"


class NodeGraph {

public:
  void evaluate() const;
  std::vector<std::unique_ptr<Node> > nodes;
  std::vector<std::unique_ptr<StreamBase> > streams;
};


#endif //NAG_NODE_GRAPH_H
