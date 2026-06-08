#ifndef NAG_ENGINE_NODE_H
#define NAG_ENGINE_NODE_H

#include <memory>
#include <string>
#include <vector>

#include "imgui.h"
#include "nlohmann/json.hpp"

#include "engine/data_type.h"
#include "engine/operation_result.h"
#include "engine/stream.h"

class CommandHistory;
struct NodeGraph;

enum PinDirection : uint8_t {
  Input,
  Output
};

enum class NodeType : uint8_t {
  Default,

  // Generators
  Constant,
  Time,
  Noise,
  Random,
  StepSequencer,
  ParticleEmitter,

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
  Compare,

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
  Ellipse,
  Rectangle2D,
  Polygon,
  TextureLoader,
  Tile,
  ParticleRenderer,

  // FX nodes
  Blur,
  ChromaticAberration,
  ColorCorrection,
  Composite,
  Displace,
  Pixelate,
  SDFShape,
  Transform,
  VintageCRT,

  // Sink
  Output,

  Count,
};

struct Pin {
  int id{};
  bool connected{false};
  PinDirection direction{Input};
  uint64_t last_seen_version{0};
  DataType data_type{DataType::Float};
  std::string name{"<unnamed>"};
  std::shared_ptr<StreamBase> stream{nullptr};

  [[nodiscard]] Stream *get_stream() { return static_cast<Stream *>(stream.get()); }
  [[nodiscard]] const Stream *get_stream() const { return static_cast<const Stream *>(stream.get()); }

  [[nodiscard]] float *get_float() {
    auto *s = static_cast<Stream *>(stream.get());
    return s ? s->as_float() : nullptr;
  }
  [[nodiscard]] const float *get_float() const {
    auto *s = static_cast<const Stream *>(stream.get());
    return s ? s->as_float() : nullptr;
  }
  [[nodiscard]] bool *get_bool() {
    auto *s = static_cast<Stream *>(stream.get());
    return s ? s->as_bool() : nullptr;
  }
  [[nodiscard]] const bool *get_bool() const {
    auto *s = static_cast<const Stream *>(stream.get());
    return s ? s->as_bool() : nullptr;
  }
  [[nodiscard]] Texture **get_texture() {
    auto *s = static_cast<Stream *>(stream.get());
    return s ? s->as_texture() : nullptr;
  }
  [[nodiscard]] Texture *const *get_texture() const {
    auto *s = static_cast<const Stream *>(stream.get());
    return s ? s->as_texture() : nullptr;
  }
  [[nodiscard]] Particles2D *get_particles() {
    auto *s = static_cast<Stream *>(stream.get());
    return s ? s->as_particles() : nullptr;
  }
  [[nodiscard]] const Particles2D *get_particles() const {
    auto *s = static_cast<const Stream *>(stream.get());
    return s ? s->as_particles() : nullptr;
  }

  void set_float(const float v) {
    if (auto *s = static_cast<Stream *>(stream.get())) {
      s->update_float(v);
    }
  }
  void set_bool(const bool v) {
    if (auto *s = static_cast<Stream *>(stream.get())) {
      s->update_bool(v);
    }
  }
  void set_texture(Texture *const v) {
    if (auto *s = static_cast<Stream *>(stream.get())) {
      s->update_texture(v);
    }
  }
  void set_particles(const Particles2D &v) {
    if (auto *s = static_cast<Stream *>(stream.get())) {
      s->update_particles(v);
    }
  }
};

struct Node {
  int id{};
  NodeType type{NodeType::Default};
  std::string name{"<unnamed>"};
  ImVec2 position{};
  std::vector<Pin> inputs;
  std::vector<Pin> outputs;

  virtual ~Node() = default;

  virtual void evaluate() = 0;

  [[nodiscard]] bool needs_evaluation() const noexcept {
    for (auto const &pin : inputs) {
      if (pin.stream && pin.stream->version != pin.last_seen_version) {
        return true;
      }
    }
    return false;
  }

  void mark_inputs_consumed() noexcept {
    for (auto &pin : inputs) {
      if (pin.stream) {
        pin.last_seen_version = pin.stream->version;
      }
    }
  }

  [[nodiscard]] virtual nlohmann::json serialize_params() const {
    return nlohmann::json::object();
  }

  [[nodiscard]] virtual OperationResult deserialize_params(const nlohmann::json &) {
    return OperationResult::ok();
  }

  virtual void draw_properties(NodeGraph &graph, CommandHistory &history) {
    (void) graph;
    (void) history;
  }

  [[nodiscard]] virtual float get_param(const std::string &param_name) const {
    (void) param_name;
    return 0.f;
  }

  void add_input(const DataType data_type = DataType::Float,
                 const std::string &pin_name = "in") {
    Pin pin;
    pin.name = pin_name;
    pin.data_type = data_type;
    pin.direction = Input;
    pin.stream = nullptr;
    inputs.push_back(pin);
  }

  void add_output(const DataType data_type = DataType::Float,
                  const std::string &pin_name = "out") {
    Pin pin;
    pin.name = pin_name;
    pin.data_type = data_type;
    pin.direction = Output;
    pin.stream = std::make_shared<Stream>(default_stream_value(data_type));
    outputs.push_back(pin);
  }

  [[nodiscard]] constexpr Pin *get_input(const std::string &input_name) noexcept {
    for (auto &pin : inputs) {
      if (pin.name == input_name) {
        return &pin;
      }
    }
    return nullptr;
  }
};

struct MultiInputNode : Node {
  static constexpr auto *const kAlphabet{"abcdefghijklmnopqrstuvwxyz"};

  MultiInputNode() = default;

  explicit MultiInputNode(const uint8_t num_inputs) {
    for (uint8_t i = 0; i < num_inputs; i++) {
      add_input();
    }
  }

  void add_input() {
    if (inputs.size() >= strlen(kAlphabet)) {
      throw std::runtime_error("Too many inputs");
    }
    Node::add_input(DataType::Float,
                    std::string(kAlphabet).substr(inputs.size(), 1));
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

#endif
