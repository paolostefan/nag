#ifndef NAG_ENGINE_MATH_NODES_H
#define NAG_ENGINE_MATH_NODES_H

#include "engine/node.h"

#include <algorithm>
#include <cmath>

// ===========================================================================
// BINARY OPERATORS (2 inputs -> 1 output)
// ===========================================================================

/**
 *  Multiplies two input streams and writes the result to the output stream.
 */
struct MultiplyNode : Node {
  MultiplyNode() {
    type = Multiply;
    name = "Multiply";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(in_a->value * in_b->value);
    mark_inputs_consumed();
  }

  static std::unique_ptr<MultiplyNode> create() {
    auto node = std::make_unique<MultiplyNode>();
    node->add_input("a");
    node->add_input("b");
    node->add_output("product");
    return node;
  }
};

/**
 * Divides two input streams and writes the result to the output stream.
 * Avoids division by zero by clamping the divisor to a small epsilon value.
 */
struct DivideNode : Node {
  static constexpr float kEpsilon{1e-6f}; // Avoid division by zero

  DivideNode() {
    type = Divide;
    name = "Divide";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in_a || !in_b || !out) {
      return;
    }

    // Protect against division by zero
    float divisor = in_b->value;
    if (std::abs(divisor) < kEpsilon) {
      divisor = kEpsilon;
    }

    out->update(in_a->value / divisor);
    mark_inputs_consumed();
  }

  static std::unique_ptr<DivideNode> create() {
    auto node = std::make_unique<DivideNode>();
    node->add_input("dividend");
    node->add_input("divisor");
    node->add_output("quotient");
    return node;
  }
};

/**
 * Adds two input streams and writes the result to the output stream.
 */
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
        if (auto *const in = dynamic_cast<Stream<float> *>(pin.stream)) {
          // This avoids crash on disconnected pins
          sum += in->value;
        }
      }

      out->update(sum);

      mark_inputs_consumed();
    }
  }
};

/**
 * Subtracts the second input stream from the first and writes the result to the output stream.
 */
struct SubtractNode : Node {
  SubtractNode() {
    type = Subtract;
    name = "Subtract";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(in_a->value - in_b->value);
    mark_inputs_consumed();
  }
};

/**
 * Computes the remainder of dividing the first input stream by the second and writes the result to the output stream.
 */
struct ModuloNode : Node {
  ModuloNode() {
    type = Modulo;
    name = "Modulo";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(std::fmod(in_a->value, in_b->value));
    mark_inputs_consumed();
  }
};

/**
 * Raises the first input stream to the power of the second and writes the result to the output stream.
 */
struct PowerNode : Node {
  PowerNode() {
    type = Power;
    name = "Power";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *base = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *exponent = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!base || !exponent || !out) {
      return;
    }

    out->update(std::pow(base->value, exponent->value));
    mark_inputs_consumed();
  }
};

/**
 * Selects the smaller of two input streams and writes the result to the output stream.
 */
struct MinNode : Node {
  MinNode() {
    type = Min;
    name = "Min";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(std::min(in_a->value, in_b->value));
    mark_inputs_consumed();
  }
};

/**
 * Selects the larger of two input streams and writes the result to the output stream.
 */
struct MaxNode : Node {
  MaxNode() {
    type = Max;
    name = "Max";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(std::max(in_a->value, in_b->value));
    mark_inputs_consumed();
  }
};

// ===========================================================================
// UNARY OPERATORS (1 input -> 1 output)
// ===========================================================================

/**
 * Takes the absolute value of the input stream and writes the result to the output stream.
 */
struct AbsNode : Node {
  AbsNode() {
    type = Abs;
    name = "Abs";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    out->update(std::abs(in->value));
    mark_inputs_consumed();
  }
};

/**
 * Rounds down to the nearest integer.
 */
struct FloorNode : Node {
  FloorNode() {
    type = Floor;
    name = "Floor";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    out->update(std::floor(in->value));
    mark_inputs_consumed();
  }
};

/**
 * Rounds up to the nearest integer.
 */
struct CeilNode : Node {
  CeilNode() {
    type = Ceil;
    name = "Ceil";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    out->update(std::ceil(in->value));
    mark_inputs_consumed();
  }
};

/**
 * Rounds to the nearest integer.
 */
struct RoundNode : Node {
  RoundNode() {
    type = Round;
    name = "Round";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    out->update(std::round(in->value));
    mark_inputs_consumed();
  }
};

/**
 *  Computes the square root of the input stream and writes the result to the output stream.
 *  Avoids NaN by clamping negative values to zero.
 */
struct SqrtNode : Node {
  SqrtNode() {
    type = Sqrt;
    name = "Sqrt";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    // Clamp to avoid NaN from negative values
    const float value = std::max(0.0f, in->value);
    out->update(std::sqrt(value));
    mark_inputs_consumed();
  }
};

/**
 * Negates the input stream and writes the result to the output stream.
 */
struct NegateNode : Node {
  NegateNode() {
    type = Negate;
    name = "Negate";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    out->update(-in->value);
    mark_inputs_consumed();
  }
};

/**
 *  Computes the sine of the input stream and writes the result to the output stream.
 */
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


/**
 * Computes the cosine of the input stream and writes the result to the output stream.
 */
struct CosNode : Node {
  CosNode() {
    type = Cos;
    name = "Cos";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    out->update(std::cos(in->value));
    mark_inputs_consumed();
  }
};

/**
 * Computes the tangent of the input stream and writes the result to the output stream.
 */
struct TanNode : Node {
  TanNode() {
    type = Tan;
    name = "Tan";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    out->update(std::tan(in->value));
    mark_inputs_consumed();
  }
};

// ===========================================================================
// SPECIAL OPERATORS
// ===========================================================================

/**
 * Maps value from [in_min, in_max] to [out_min, out_max].
 */
struct RemapNode : Node {
  float in_min{0.0f};
  float in_max{1.0f};
  float out_min{0.0f};
  float out_max{1.0f};

  RemapNode() {
    type = Remap;
    name = "Remap";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    // Avoid division by zero
    const float in_range = in_max - in_min;
    if (std::abs(in_range) < 1e-6f) {
      out->update(out_min);
      mark_inputs_consumed();
      return;
    }

    // Linear mapping
    const float t = (in->value - in_min) / in_range;
    const float result = out_min + t * (out_max - out_min);

    out->update(result);
    mark_inputs_consumed();
  }

  static std::unique_ptr<RemapNode> create(const float in_min = 0.0f,
                                           const float in_max = 1.0f,
                                           const float out_min = 0.0f,
                                           const float out_max = 1.0f) {
    auto node = std::make_unique<RemapNode>();
    node->in_min = in_min;
    node->in_max = in_max;
    node->out_min = out_min;
    node->out_max = out_max;
    node->add_input("in");
    node->add_output("out");
    return node;
  }
};

/**
 * Clamps the input stream to the specified range and writes the result to the output stream.
 */
struct ClampNode : Node {
  float min_value{0.0f};
  float max_value{1.0f};

  ClampNode() {
    type = Clamp;
    name = "Clamp";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    const float clamped = std::clamp(in->value, min_value, max_value);
    out->update(clamped);
    mark_inputs_consumed();
  }
};

/**
 * Interpolates between two input streams (A and B) based on a third input stream (0-1).
 */
struct LerpNode : Node {
  LerpNode() {
    type = Lerp;
    name = "Lerp";
  }

  void evaluate() override {
    if (inputs.size() < 3 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream);
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream);
    const auto *in_t = dynamic_cast<Stream<float> *>(inputs[2].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in_a || !in_b || !in_t || !out) {
      return;
    }

    float t = std::clamp(in_t->value, 0.0f, 1.0f);
    float result = in_a->value + t * (in_b->value - in_a->value);

    out->update(result);
    mark_inputs_consumed();
  }
};

/**
 * Smooth interpolation with ease in/out (Hermite interpolation)
 */
struct SmoothStepNode : Node {
  float edge0{0.0f};
  float edge1{1.0f};

  SmoothStepNode() {
    type = SmoothStep;
    name = "SmoothStep";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream);
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream);

    if (!in || !out) {
      return;
    }

    // Clamp t to [0, 1]
    const float t = std::clamp((in->value - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    // Hermite interpolation: 3t² - 2t³
    const float smooth = t * t * (3.0f - 2.0f * t);

    out->update(smooth);
    mark_inputs_consumed();
  }
};


#endif //NAG_ENGINE_MATH_NODES_H
