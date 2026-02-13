#ifndef NAG_ENGINE_NODE_H
#define NAG_ENGINE_NODE_H

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

  // Generators
  Constant,
  Time,
  Noise,
  Random,
  StepSequencer,

  // Unary math operators
  Abs,
  Floor,
  Ceil,
  Round,
  Sqrt,
  Negate,

  // Trigonometric functions
  Sin,
  Cos,
  Tan,

  // Binary math operators
  Subtract,
  Multiply,
  Divide,
  Modulo,
  Power,

  // N-ary math operators
  Add,
  Min,
  Max,

  // "Special" operators
  Remap,
  Clamp,
  Lerp,
  SmoothStep,

  // Temporal modifiers
  LFO,
  Envelope,
  Delay,
  Smoother,

  // Visual nodes
  ClearColor,
};

struct Pin {
  uint64_t id{};
  std::string name{"<unnamed>"};

  PinDirection direction{Input};
  std::shared_ptr<StreamBase> stream{nullptr};

  uint64_t last_seen_version{0}; // Used only by input pins
};

struct Node {
  uint64_t id{};
  NodeType type{Default};
  std::string name{"<unnamed>"};

  ImVec2 position{};

  // TODO: make these std:array's (a node can't have 2000+ pins)
  std::vector<Pin> inputs;
  std::vector<Pin> outputs;

  virtual ~Node() = default;

  virtual void evaluate() = 0;

  [[nodiscard]] bool needs_evaluation() const noexcept {
    for (auto const &pin: inputs) {
      if (pin.stream && pin.stream->version != pin.last_seen_version) {
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
      if (pin.stream) {
        pin.last_seen_version = pin.stream->version;
      }
    }
  }

  void add_input(const std::string &pin_name = "in") {
    inputs.push_back({0, pin_name, Input, nullptr, 0});
  }

  void add_output(const std::string &pin_name = "out") {
    outputs.push_back({0, pin_name, Output, nullptr, 0});
  }
};

#endif //NAG_ENGINE_NODE_H
