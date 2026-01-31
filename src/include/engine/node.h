#ifndef NAG_NODE_H
#define NAG_NODE_H

#include <cmath>
#include <vector>

#include "imgui.h"

#include "engine/stream.h"

enum PinDirection:uint8_t {
  Input,
  Output
};

struct Pin {
  PinDirection direction;
  StreamBase *stream{nullptr};
  uint64_t last_seen_version{0}; // Used only by input pins
  const char *name{nullptr};
};

struct Node {
  std::vector<Pin> inputs;
  std::vector<Pin> outputs;

  uint64_t id{};
  std::string name{"<unnamed>"};

  ImVec2 position{};
  ImVec2 size{120, 80};

  Node() = default;

  virtual ~Node() = default;

  virtual void evaluate() = 0;

  [[nodiscard]] bool needs_evaluation() const {
    for (auto const &pin: inputs) {
      if (pin.stream->version != pin.last_seen_version) {
        return true;
      }
    }
    return false;
  }

  void mark_inputs_consumed() {
    for (auto &pin: inputs) {
      pin.last_seen_version = pin.stream->version;
    }
  }
};

// ===========================================================================

struct AddFloatNode : Node {
  AddFloatNode(Stream<float> *in_a, Stream<float> *in_b, Stream<float> *out) {
    inputs.push_back({Input, in_a, 0, "a"});
    inputs.push_back({Input, in_b, 0, "b"});
    outputs.push_back({Output, out, 0, "out"});
  }

  void evaluate() override {
    auto const *const in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto const *const in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *const out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (in_a && in_b && out) {
      out->update(in_a->value + in_b->value);

      mark_inputs_consumed();
    }
  }
};

// ===========================================================================

struct SinNode : Node {
  float amplitude{1.0f};
  float frequency{1.0f};
  float phase{0.0f};


  SinNode(Stream<float> *in, Stream<float> *_out) {
    inputs.push_back({Input, in, 0, "time"});
    outputs.push_back({Output, _out, 0, "out"});
  }

  SinNode(Stream<float> *in, Stream<float> *_out,
          const float _amplitude, float _frequency = 1.0f, float _phase = 0.0f)
    : SinNode(in, _out) {
    amplitude = _amplitude;
    frequency = _frequency;
    phase = _phase;
  }

  void evaluate() override {
    auto *time = static_cast<Stream<float> *>(inputs[0].stream);
    auto *out = static_cast<Stream<float> *>(outputs[0].stream);

    out->update(amplitude * std::sin(frequency * time->value + phase));
    mark_inputs_consumed();
  }
};

#endif //NAG_NODE_H
