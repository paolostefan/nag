#ifndef NAG_ENGINE_NODE_H
#define NAG_ENGINE_NODE_H

#include <memory>
#include <string>
#include <vector>

#include "imgui.h"
#include "nlohmann/json.hpp"

#include "engine/data_type.h"
#include "engine/node_type.h"
#include "engine/operation_result.h"
#include "engine/stream.h"

class CommandHistory;
struct NodeGraph;

enum PinDirection : uint8_t {
  Input,
  Output
};

struct Pin {
  std::string name{"<unnamed>"};
  std::shared_ptr<StreamBase> stream{nullptr};
  uint64_t last_seen_version{0};
  int id{};
  PinDirection direction{Input};
  DataType data_type{DataType::Float};
  bool connected{false};
  uint8_t pad{}; // Used as general purpose field in particular pins

  [[nodiscard]] Stream *get_stream() { return static_cast<Stream *>(stream.get()); }
  [[nodiscard]] const Stream *get_stream() const { return static_cast<const Stream *>(stream.get()); }

  [[nodiscard]] float *get_float() {
    auto *s = (Stream *) stream.get();
    return s ? s->as_float() : nullptr;
  }

  [[nodiscard]] const float *get_float() const {
    auto *s = (const Stream *) stream.get();
    return s ? s->as_float() : nullptr;
  }

  [[nodiscard]] bool *get_bool() {
    auto *s = (Stream *) stream.get();
    return s ? s->as_bool() : nullptr;
  }

  [[nodiscard]] const bool *get_bool() const {
    auto *s = (const Stream *) stream.get();
    return s ? s->as_bool() : nullptr;
  }

  [[nodiscard]] Texture **get_texture() {
    auto *s = (Stream *) stream.get();
    return s ? s->as_texture() : nullptr;
  }

  [[nodiscard]] Texture *const *get_texture() const {
    auto *s = (const Stream *) stream.get();
    return s ? s->as_texture() : nullptr;
  }

  [[nodiscard]] Particles2D *get_particles() {
    auto *s = (Stream *) stream.get();
    return s ? s->as_particles() : nullptr;
  }

  [[nodiscard]] const Particles2D *get_particles() const {
    auto *s = (const Stream *) stream.get();
    return s ? s->as_particles() : nullptr;
  }

  void set_float(const float v) const {
    if (auto *s = (Stream *) stream.get()) {
      s->update_float(v);
    }
  }

  void set_bool(const bool v) const {
    if (auto *s = (Stream *) stream.get()) {
      s->update_bool(v);
    }
  }

  void set_texture(Texture *const v) const {
    if (auto *s = (Stream *) stream.get()) {
      s->update_texture(v);
    }
  }

  void set_particles(const Particles2D &v) const {
    if (auto *s = (Stream *) stream.get()) {
      s->update_particles(v);
    }
  }
};

/**
 * @struct Node
 * @brief Base class for all nodes in the graph.
 */
struct Node {
  std::string name{"<unnamed>"};
  std::vector<Pin> inputs;
  std::vector<Pin> outputs;
  ImVec2 position{};
  int id{};
  NodeType type{NodeType::Default};
  uint8_t pad[3]{};

  virtual ~Node() = default;

  virtual void evaluate() = 0;

  [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;

  [[nodiscard]] bool needs_evaluation() const noexcept {
    for (auto const &pin: inputs) {
      if (pin.stream && pin.stream->version != pin.last_seen_version) {
        return true;
      }
    }
    return false;
  }

  void mark_inputs_consumed() noexcept {
    for (auto &pin: inputs) {
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

  Pin *add_input(const DataType data_type = DataType::Float,
                 const std::string &pin_name = "in") {
    Pin pin;
    pin.name = pin_name;
    pin.data_type = data_type;
    pin.direction = Input;
    pin.stream = nullptr;
    inputs.push_back(pin);
    return &inputs.back();
  }

  Pin *add_output(const DataType data_type = DataType::Float,
                  const std::string &pin_name = "out") {
    Pin pin;
    pin.name = pin_name;
    pin.data_type = data_type;
    pin.direction = Output;
    pin.stream = std::make_shared<Stream>(default_stream_value(data_type));
    outputs.push_back(pin);

    return &outputs.back();
  }

  [[nodiscard]] constexpr Pin *get_input(const std::string &input_name) noexcept {
    for (auto &pin: inputs) {
      if (pin.name == input_name) {
        return &pin;
      }
    }
    return nullptr;
  }

  [[nodiscard]] constexpr Pin *get_input_by_id(const int input_id) noexcept {
    for (auto &pin: inputs) {
      if (pin.id == input_id) {
        return &pin;
      }
    }

    return nullptr;
  }

  void read_pin_to(const char *const pin_name, float &destination) {
    if (const Pin *pin = get_input(pin_name)) {
      if (const float *val_ptr = pin->get_float()) {
        destination = *val_ptr;
      }
    }
  }

  // void read_pin_to(const int pin_id, float &destination) {
  //   if (const Pin *pin = get_input_by_id(pin_id)) {
  //     if(const float *val_ptr = pin->get_float()) {
  //       destination = *val_ptr;
  //     }
  //   }
  // }

  Pin *remove_input(const size_t index) {
    if (index >= inputs.size()) {
      return nullptr;
    }
    const auto it = inputs.begin() + index;
    auto *removed_pin = new Pin(std::move(*it));
    inputs.erase(it);
    return removed_pin;
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
