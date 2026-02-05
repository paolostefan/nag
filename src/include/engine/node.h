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

enum NodeType:uint8_t {
  Default,
  Add,
  Sin,
};

struct Pin {
  uint64_t id{};
  std::string name{"<unnamed>"};

  ImVec2 position{};

  PinDirection direction{Input};
  StreamBase *stream{nullptr};

  uint64_t last_seen_version{0}; // Used only by input pins
};

struct Node {
  uint64_t id{};
  NodeType type{Default};
  std::string name{"<unnamed>"};

  ImVec2 position{};

  std::vector<Pin> inputs;
  std::vector<Pin> outputs;

  virtual ~Node() = default;

  virtual void evaluate() = 0;

  [[nodiscard]] bool needs_evaluation() const noexcept {
    for (auto const &pin: inputs) {
      if (pin.stream->version != pin.last_seen_version) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Marks all input pins as consumed by updating their last seen version.
   *
   * This method updates the last_seen_version of each input pin to match the current
   * version of its connected stream. This is typically called after evaluate() to
   * indicate that the node has processed the current input values.
   */
  void mark_inputs_consumed() noexcept {
    for (auto &pin: inputs) {
      pin.last_seen_version = pin.stream->version;
    }
  }

  void add_input(const std::string &pin_name = "in") {
    inputs.push_back({0, pin_name, {}, Input, nullptr, 0});
  }

  void add_output(const std::string &pin_name = "out") {
    outputs.push_back({0, pin_name, {}, Output, nullptr, 0});
  }
};

// ===========================================================================

struct AddFloatNode : Node {
  static constexpr auto *const kAlphabet{"abcdefghijklmnopqrstuvwxyz"};

  AddFloatNode() {
    type = Add;
    name = "Add";
  }

  /**
   * Add an input stream to this node.
   * @note AddNode allows up to 26 input streams.
   */
  void add_input() {
    if (inputs.size() >= strlen(kAlphabet)) {
      throw std::runtime_error("Too many inputs");
    }

    // Add an input pin with name = nth letter of kAlphabet
    Node::add_input(std::string(kAlphabet).substr(inputs.size(), 1));
  }

  /**
   * Sum all input streams and write the result to the output stream.
   */
  void evaluate() override {
    if (!outputs.empty()) {
      auto *const out = dynamic_cast<Stream<float> *>(outputs[0].stream);
      if (!out) {
        throw std::runtime_error("Invalid output pin");
      }

      float sum = 0.0f;
      for (auto const &pin: inputs) {
        auto const *const in = dynamic_cast<Stream<float> *>(pin.stream);
        if (!in) {
          throw std::runtime_error("Invalid input pin");
        }
        sum += in->value;
      }

      out->update(sum);

      mark_inputs_consumed();
    }
  }
};

// ===========================================================================

struct SinNode : Node {
  float amplitude{1.0f};
  float frequency{1.0f};
  float phase{0.0f};

  explicit SinNode(const float _amplitude = 1.0f, const float _frequency = 1.0f, const float _phase = 0.0f)
    : amplitude(_amplitude), frequency(_frequency), phase(_phase) {
    type = Sin;
    name = "Sin";
  }

  void evaluate() override {
    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      throw std::runtime_error("invalid connections");
    }

    out->update(amplitude * std::sin(frequency * in->value + phase));
    mark_inputs_consumed();
  }
};

#endif //NAG_NODE_H
