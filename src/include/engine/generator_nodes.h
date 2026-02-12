#ifndef NAG_ENGINE_GENERATOR_NODES_H
#define NAG_ENGINE_GENERATOR_NODES_H

#include <cmath>
#include <random>

#include "engine/node.h"

/**
 * Node that outputs a constant float value.
 */
struct ConstantFloatNode : Node {
  float value{0.0f};

  explicit ConstantFloatNode(const float _value = 0.0f)
    : value(_value) {
    type = Constant;
    name = "Constant";
  }

  void evaluate() override {
    if (!outputs.empty()) {
      if (const auto out = dynamic_cast<Stream<float> *>(outputs[0].stream.get())) {
        out->update(value);
      }
    }
  }

  static std::unique_ptr<Node> create(const float _value = 0.0f) {
    auto node = std::make_unique<ConstantFloatNode>(_value);
    node->add_output("out");
    return node;
  }
};

struct TimeNode : Node {
  float time{0.0f};

  explicit TimeNode() {
    type = Time;
    name = "Time";
  }

  constexpr void evaluate() override {
    if (!outputs.empty()) {
      if (const auto out = dynamic_cast<Stream<float> *>(outputs[0].stream.get())) {
        out->update(time);
      }
    }
  }

  constexpr void step(const float dt) {
    time += dt;
    evaluate();
  }

  static std::unique_ptr<Node> create() {
    auto node = std::make_unique<TimeNode>();
    node->add_output("time");
    node->outputs[0].stream = std::make_shared<Stream<float> >();

    return node;
  }
};


/**
 * Node that outputs a noise value based on a sinusoidal approximation.
 */
struct NoiseNode : Node {
  float frequency{1.0f};
  float amplitude{1.0f};
  int octaves{1};
  float persistence{0.5f};


  NoiseNode() {
    type = Noise;
    name = "Noise";
  }

  static std::unique_ptr<NoiseNode> create(const float frequency = 1.0f,
                                           const float amplitude = 1.0f,
                                           const int octaves = 1,
                                           const float persistence = 0.5f) {
    auto node = std::make_unique<NoiseNode>();
    node->frequency = frequency;
    node->amplitude = amplitude;
    node->octaves = octaves;
    node->persistence = persistence;
    node->add_input("x");
    node->add_output("noise");
    return node;
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    float result = 0.0f;
    float amp = amplitude;
    float freq = frequency;

    for (int i = 0; i < octaves; ++i) {
      result += simple_noise(in->value * freq) * amp;
      amp *= persistence;
      freq *= 2.0f;
    }

    out->update(result);
    mark_inputs_consumed();
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
  float min_value{0.0f};
  float max_value{1.0f};
  int seed{42};

  uint64_t last_trigger_version{0};
  std::mt19937 rng;
  std::uniform_real_distribution<float> dist;

  RandomNode() {
    type = Random;
    name = "Random";
    rng.seed(seed);
    dist = std::uniform_real_distribution(min_value, max_value);
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

    const auto *trigger = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!trigger || !out) {
      return;
    }

    // Generate new value only when the trigger changes
    if (trigger->version != last_trigger_version) {
      float random_value = dist(rng);
      out->update(random_value);
      last_trigger_version = trigger->version;
    }

    mark_inputs_consumed();
  }
};

/**
 *  Node that outputs a sequence of values based on a trigger input.
 */
struct StepSequencerNode : Node {
  std::vector<float> steps{0.0f, 0.5f, 1.0f, 0.5f}; // Default pattern
  uint64_t current_step{0};
  uint64_t last_trigger_version{0};

  StepSequencerNode() {
    type = StepSequencer;
    name = "StepSequencer";
  }

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

    const auto *trigger = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!trigger || !out) {
      return;
    }

    // Advance step only when the trigger changes
    if (trigger->version != last_trigger_version) {
      current_step = (current_step + 1) % steps.size();
      last_trigger_version = trigger->version;
    }

    out->update(steps[current_step]);
    mark_inputs_consumed();
  }
};

#endif //NAG_ENGINE_GENERATOR_NODES_H
