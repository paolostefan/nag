#ifndef NAG_ENGINE_NODE_H
#define NAG_ENGINE_NODE_H

#include <vector>

#include "imgui.h"
#include "nlohmann/json.hpp"

#include "engine/operation_result.h"
#include "engine/stream.h"

class CommandHistory;
struct NodeGraph;

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

  /// Whether this pin is currently connected to another pin via a link.
  bool connected{false};

  // Whether this is an input or output pin
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

  // TODO: make inputs & outputs std:array's of a reasonable size (20? a node should not have 100+ pins)
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
   * @return Result indicating success or error
   */
  [[nodiscard]] virtual OperationResult deserialize_params(const nlohmann::json &) {
    return OperationResult::ok();
  }

  /// @brief Render ImGui widgets for this node's parameters.
  ///
  /// Called every frame by NodePropertiesPanel when this node is selected.
  /// The default implementation is a no-op; concrete nodes that expose
  /// editable parameters should override this.
  ///
  /// Implementations should use the PropertyWidget helpers to ensure that
  /// edits are recorded in CommandHistory with correct undo/redo semantics.
  ///
  /// @param graph    The active node graph (forwarded to `history.execute()`).
  /// @param history  The command history.
  virtual void draw_properties(NodeGraph &graph, CommandHistory &history) {
    // Default: no parameters to show.
    (void) graph;
    (void) history;
  }

  [[nodiscard]] virtual float get_param(const std::string &param_name) const {
    // Default implementation: no parameters, return 0.
    (void) param_name;
    return 0.0f;
  }

  // Add a typed input Pin to this Node
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

  /**
   * @brief Get a pointer to an input pin by name.
   * @param input_name
   * @return The pin referenced by this name, or nullptr if not found.
   * Note that the returned pointer may be invalidated if the node's inputs are modified
   * (e.g. by add_input).
   */
  Pin *get_input(const std::string &input_name) {
    for (auto &pin: inputs) {
      if (pin.name == input_name) {
        return &pin;
      }
    }
    return nullptr;
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
      for (size_t i = 0; i < inputs_size; i++) {
        add_input();
      }
    }
    return OperationResult::ok();
  }
};

#endif //NAG_ENGINE_NODE_H
