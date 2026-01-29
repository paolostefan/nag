#ifndef NAG_NODE_H
#define NAG_NODE_H

#include <vector>

#include "engine/stream.h"

struct Node {
  Node() = default;

  explicit Node(std::vector<StreamBase *> inputs, std::vector<StreamBase *> outputs)
    : inputs(std::move(inputs)), outputs(std::move(outputs)) {
  }

  virtual ~Node() = default;

  std::vector<StreamBase *> inputs;
  std::vector<StreamBase *> outputs;

  virtual void evaluate() = 0;
};

struct AddFloatNode : public Node {
  AddFloatNode()
      : Node({nullptr, nullptr}, {nullptr}) {
  }

  AddFloatNode(Stream<float> *in_a, Stream<float> *in_b, Stream<float> *out)
      : Node({in_a, in_b}, {out}) {
  }

  void evaluate() override {
    auto const *const in_a = dynamic_cast<Stream<float> *>(inputs[0]);
    auto const *const in_b = dynamic_cast<Stream<float> *>(inputs[1]);

    if (auto *const out = dynamic_cast<Stream<float> *>(outputs[0]); in_a && in_b && out) {
      out->value = in_a->value + in_b->value;
    }
  }
};

#endif //NAG_NODE_H
