#ifndef NAG_ENGINE_GENERATOR_NODES_H
#define NAG_ENGINE_GENERATOR_NODES_H

#include <cmath>
#include <random>

#include "engine/nodes/node.h"

/**
 * Node that outputs a constant float value.
 */
struct ConstantFloatNode : Node {
  float value{0.f};

  explicit ConstantFloatNode(const float _value = 0.f)
    : value(_value) {
    type = NodeType::Constant;
    name = "Constant";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Constant"; }

  void evaluate() override {
    if (!outputs.empty()) {
      outputs[0].set_float(value);
    }
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["value"] = value;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    if (j.contains("value")) value = j["value"];
    return OperationResult::ok();
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "value") return value;
    return 0.f;
  }

  static std::unique_ptr<Node> create(const float _value = 0.f) {
    auto node = std::make_unique<ConstantFloatNode>(_value);
    node->add_output(DataType::Float, "out");
    return node;
  }
};

/**
 * @struct TimeNode
 * @brief Node that outputs the elapsed time in seconds since the graph started.
 */
struct TimeNode : Node {
  float time{0.f};

  explicit TimeNode() {
    type = NodeType::Time;
    name = "Time";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Time"; }

  constexpr void evaluate() override {
    if (!outputs.empty()) {
      outputs[0].set_float(time);
    }
  }

  constexpr void step(const float dt) {
    time += dt;
    evaluate();
  }

  static std::unique_ptr<Node> create() {
    auto node = std::make_unique<TimeNode>();
    node->add_output(DataType::Float, "time");

    return node;
  }
};

/**
 * Node that outputs a noise value based on a sinusoidal approximation.
 */
struct NoiseNode : Node {
  float frequency{1.f};
  float amplitude{1.f};
  int octaves{1};
  float persistence{0.5f};

  NoiseNode() {
    type = NodeType::Noise;
    name = "Noise";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Noise"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in) { return; }

    float result = 0.f;
    float amp = amplitude;
    float freq = frequency;

    for (int i = 0; i < octaves; ++i) {
      result += simple_noise((*in) * freq) * amp;
      amp *= persistence;
      freq *= 2.f;
    }

    outputs[0].set_float(result);
    mark_inputs_consumed();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["frequency"] = frequency;
    j["amplitude"] = amplitude;
    j["octaves"] = octaves;
    j["persistence"] = persistence;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (j.contains("frequency")) frequency = j["frequency"];
      if (j.contains("amplitude")) amplitude = j["amplitude"];
      if (j.contains("octaves")) octaves = j["octaves"];
      if (j.contains("persistence")) persistence = j["persistence"];
      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize Noise params: ") + e.what()
      );
    }
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "frequency") return frequency;
    if (param_name == "amplitude") return amplitude;
    if (param_name == "octaves") return static_cast<float>(octaves);
    if (param_name == "persistence") return persistence;
    return 0.f;
  }

  static std::unique_ptr<NoiseNode> create(const float frequency = 1.f,
                                           const float amplitude = 1.f,
                                           const int octaves = 1,
                                           const float persistence = 0.5f) {
    auto node = std::make_unique<NoiseNode>();
    node->frequency = frequency;
    node->amplitude = amplitude;
    node->octaves = octaves;
    node->persistence = persistence;
    node->add_input(DataType::Float, "x");
    node->add_output(DataType::Float, "noise");
    return node;
  }

private:
  // Simplified noise function (sinusoidal approximation)
  // TODO: replace with proper Perlin/Simplex noise in the future
  static float simple_noise(const float x) {
    return std::sin(x * 3.14159f) *
           std::cos(x * 1.41421f) *
           std::sin(x * 2.71828f);
  }
};

/**
 * Node that outputs a random float value within a specified range.
 */
struct RandomNode : Node {
  float min_value{0.f};
  float max_value{1.f};
  int seed{42};

  uint64_t last_trigger_version{0};
  std::mt19937 rng;
  std::uniform_real_distribution<float> dist;

  RandomNode() {
    type = NodeType::Random;
    name = "Random";
  };

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Random"; }

  RandomNode(const float min_value_,
             const float max_value_,
             const int seed_) : RandomNode() {
    set_seed(seed_);
    set_range(min_value_, max_value_);
  }

  void set_seed(const int new_seed) {
    seed = new_seed;
    rng.seed(seed);
  }

  void set_range(const float min, const float max) {
    min_value = min;
    max_value = max;
    dist = std::uniform_real_distribution(min_value, max_value);
  }

  void evaluate() override {
    // Input 0: trigger - when changes, generates new random value
    // Output 0: random value

    if (inputs.empty() || outputs.empty()) {
      return;
    }

    if (const float *trigger = inputs[0].get_float(); !trigger || !outputs[0].get_float()) { return; }

    // Generate new value only when the trigger changes
    if (inputs[0].stream->version != last_trigger_version) {
      const float random_value = dist(rng);
      outputs[0].set_float(random_value);
      last_trigger_version = inputs[0].stream->version;
    }

    mark_inputs_consumed();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["min_value"] = min_value;
    j["max_value"] = max_value;
    j["seed"] = seed;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (j.contains("min_value")) min_value = j["min_value"];
      if (j.contains("max_value")) max_value = j["max_value"];
      if (j.contains("seed")) seed = j["seed"];
      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize RandomNode params: ") + e.what()
      );
    }
  }

  static std::unique_ptr<RandomNode> create(
    const float min_value_ = 0.f,
    const float max_value_ = 1.f,
    const int seed_ = 42
  ) {
    auto node = std::make_unique<RandomNode>(min_value_, max_value_, seed_);

    node->add_input(DataType::Float, "trigger");
    node->add_output(DataType::Float, "random");
    return node;
  }
};

/**
 *  Node that outputs a sequence of values based on a trigger input.
 */
struct StepSequencerNode : Node {
  std::vector<float> steps{0.f, 0.5f, 1.f, 0.5f}; // Default pattern
  uint64_t current_step{0};
  uint64_t last_trigger_version{0};

  StepSequencerNode() {
    type = NodeType::StepSequencer;
    name = "StepSequencer";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Step Sequencer"; }

  void set_steps(const std::vector<float> &new_steps) {
    if (!new_steps.empty()) {
      steps = new_steps;
      current_step = 0;
    }
  }

  void evaluate() override {
    // Input 0: trigger - advances step when changes
    // Output 0: current step value

    if (inputs.empty() || outputs.empty()) {
      return;
    }

    if (const float *trigger = inputs[0].get_float(); !trigger || !outputs[0].get_float()) { return; }

    // Advance step only when the trigger changes
    if (inputs[0].stream->version != last_trigger_version) {
      current_step = (current_step + 1) % steps.size();
      last_trigger_version = inputs[0].stream->version;
    }

    outputs[0].set_float(steps[current_step]);
    mark_inputs_consumed();
  }

  static std::unique_ptr<StepSequencerNode> create(const std::vector<float> &steps = {}) {
    auto node = std::make_unique<StepSequencerNode>();
    node->set_steps(steps);
    return node;
  }
};

#endif //NAG_ENGINE_GENERATOR_NODES_H
