#ifndef NAG_NODE_H
#define NAG_NODE_H

#include <vector>

#include "engine/stream.h"

struct Node {
  Node() = default;

  explicit Node(std::vector<StreamBase *> _inputs,
                std::vector<StreamBase *> _outputs)
    : last_seen_input_versions(_inputs.size(), 0),
      inputs(std::move(_inputs)),
      outputs(std::move(_outputs)) {
  }

  virtual ~Node() = default;

  std::vector<uint64_t> last_seen_input_versions;

  std::vector<StreamBase *> inputs;
  std::vector<StreamBase *> outputs;

  virtual void evaluate() = 0;

  [[nodiscard]] bool needs_evaluation() const {
    for (size_t i = 0; i < inputs.size(); i++) {
      auto *const input = inputs[i];
      if (input->version != last_seen_input_versions[i]) {
        return true;
      }
    }
    return false;
  }

  void mark_inputs_consumed() {
    for (size_t i = 0; i < inputs.size(); i++) {
      last_seen_input_versions[i] = inputs[i]->version;
    }
  }
};

// ===========================================================================

struct AddFloatNode : Node {
  AddFloatNode(Stream<float> *in_a, Stream<float> *in_b, Stream<float> *out)
    : Node({in_a, in_b}, {out}) {
  }

  void evaluate() override {
    auto *const in_a = dynamic_cast<Stream<float> *>(inputs[0]);
    auto *const in_b = dynamic_cast<Stream<float> *>(inputs[1]);
    auto *const out = dynamic_cast<Stream<float> *>(outputs[0]);

    if (in_a && in_b && out) {
      out->update(in_a->value + in_b->value);

      mark_inputs_consumed();
    }
  }
};

// ===========================================================================

struct SinNode : Node {
  Stream<float> *time{};
  Stream<float> *out{};

  float amplitude{1.0f};
  float frequency{1.0f};
  float phase{0.0f};


  SinNode(Stream<float> *in, Stream<float> *_out)
    : Node({in}, {_out}) {
    time = in;
    out = _out;
  }

  SinNode(Stream<float> *in, Stream<float> *_out,
          const float _amplitude = 1.0f, float _frequency = 1.0f, float _phase = 0.0f)
    : Node({in}, {_out}) {
    time = in;
    out = _out;
    amplitude = _amplitude;
    frequency = _frequency;
    phase = _phase;
  }

  void evaluate() override {
    out->update(amplitude * std::sin(frequency * time->value + phase));
    mark_inputs_consumed();
  }
};

#endif //NAG_NODE_H
