#include "engine/node_graph.h"

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
