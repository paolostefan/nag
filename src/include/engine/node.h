#ifndef NAG_ENGINE_NODE_H
#define NAG_ENGINE_NODE_H

#include <cmath>
#include <vector>

#include "imgui.h"
#include "nlohmann/json.hpp"

#include "engine/operation_result.h"
#include "engine/stream.h"

enum PinDirection:uint8_t {
  Input,
  Output
};

enum class NodeType:uint8_t {
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
  Gradient,
  Circle,
  Rectangle2D,
  Composite,
  Blur,
  ChromaticAberration,
  Pixelate,

  // Sink
  Output,
};

struct Pin {
  // Global pin id graph-wise
  int id{};

  PinDirection direction{Input};

  // Used only by input pins
  uint64_t last_seen_version{0};

  // Declared pin type
  const std::type_info *data_type{&typeid(void)};

  std::string name{"<unnamed>"};

  std::shared_ptr<StreamBase> stream{nullptr};

  // Type-safe value access
  template<typename T>
  [[nodiscard]] T *try_get_value() const {
    if (!stream || stream->type() != typeid(T)) {
      return nullptr;
    }

    return &static_cast<Stream<T> *>(stream.get())->value;
  }

  template<typename T>
  [[nodiscard]] const T *try_get_value() const {
    if (!stream || stream->type() != typeid(T)) {
      return nullptr;
    }

    return &static_cast<Stream<T> *>(stream.get())->value;
  }

  // Check compatibility for UI
  [[nodiscard]] const char *get_type_name() const {
    return data_type->name();
  }
};

struct Node {
  int id{};
  NodeType type{NodeType::Default};

  std::string name{"<unnamed>"};

  ImVec2 position{};

  // TODO: make these std:array's of a reasonable size (10? 20? a node can't have 2000+ pins)
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

  /**
  * @brief Serialize node-specific parameters to JSON.
  *
  * Override this in derived classes to save custom parameters.
  * Base implementation returns empty object.
  *
  * @return JSON object with node parameters
  */
  [[nodiscard]] virtual nlohmann::json serialize_params() const {
    return nlohmann::json::object();
  }

  /**
   * @brief Deserialize node-specific parameters from JSON.
   *
   * Override this in derived classes to load custom parameters.
   * Base implementation does nothing and returns success.
   *
   * @param j JSON object with node parameters
   * @return Result indicating success or error
   */
  [[nodiscard]] virtual OperationResult deserialize_params(const nlohmann::json &j) {
    return OperationResult::ok();
  }

  // Add typed input
  template<typename T>
  void add_typed_input(const std::string &pin_name = "in") {
    Pin pin;
    pin.name = pin_name;
    pin.data_type = &typeid(T);
    pin.direction = Input;
    pin.stream = nullptr;
    inputs.push_back(pin);
  }

  void add_input(const std::string &pin_name = "in") {
    add_typed_input<float>(pin_name);
  }

  template<typename T>
  void add_typed_output(const std::string &pin_name = "out") {
    Pin pin;
    pin.name = pin_name;
    pin.direction = Output;
    pin.data_type = &typeid(T);
    pin.stream = std::make_shared<Stream<T> >();
    outputs.push_back(pin);
  }

  void add_output(const std::string &pin_name = "out") {
    add_typed_output<float>(pin_name);
  }
}; // Node

/**
 * Node that supports up to 26 input pins (named a...z)
 */
struct MultiInputNode : Node {
  static constexpr auto *const kAlphabet{"abcdefghijklmnopqrstuvwxyz"};

  MultiInputNode() = default;

  explicit MultiInputNode(const uint8_t num_inputs) {
    for (uint8_t i = 0; i < num_inputs; i++) {
      add_input();
    }
  }

  /**
   * Add an input stream to this node.
   * @note MultiInputNode allows up to 26 input streams.
   */
  void add_input() {
    if (inputs.size() >= strlen(kAlphabet)) {
      throw std::runtime_error("Too many inputs");
    }

    // Add an input pin with name = nth letter of kAlphabet
    Node::add_input(std::string(kAlphabet).substr(inputs.size(), 1));
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["inputs_size"] = inputs.size();
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    if (j.contains("inputs_size")) {
      const size_t inputs_size = j["inputs_size"];
      inputs.clear();
      for (size_t i=0; i<inputs_size; i++) {
        add_input();
      }
    }
    return OperationResult::ok();
  }
};

#endif //NAG_ENGINE_NODE_H
