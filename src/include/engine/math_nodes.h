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
struct MultiplyNode : MultiInputNode {
  explicit MultiplyNode() : MultiInputNode(2) {
    type = Multiply;
    name = "Multiply";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(in_a->value * in_b->value);
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<MultiplyNode> create() {
    auto node = std::make_unique<MultiplyNode>();
    node->add_output("product");
    return std::move(node);
  }
};

/**
 * Divides two input streams and writes the result to the output stream.
 * Avoids division by zero by clamping the divisor to a small epsilon value.
 */
struct DivideNode : MultiInputNode {
  static constexpr float kEpsilon{1e-6f}; // Avoid division by zero

  explicit DivideNode() : MultiInputNode(2) {
    type = Divide;
    name = "Divide";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

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

  [[nodiscard]] static std::unique_ptr<DivideNode> create() {
    auto node = std::make_unique<DivideNode>();
    node->add_output("quotient");
    return std::move(node);
  }
};

/**
 * Adds two input streams and writes the result to the output stream.
 */
struct AddNode : MultiInputNode {
  explicit AddNode(const size_t num_inputs = 2) : MultiInputNode(num_inputs) {
    type = Add;
    name = "Add";
  }

  /**
   * Sum all input streams and write the result to the output stream.
   */
  void evaluate() override {
    if (!outputs.empty()) {
      auto *const out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());
      if (!out) {
        throw std::runtime_error("Invalid output pin");
      }

      float sum = 0.0f;
      for (const auto &pin: inputs) {
        if (const auto *const in = dynamic_cast<Stream<float> *>(pin.stream.get())) {
          // This avoids crash on disconnected pins
          sum += in->value;
        }
      }

      out->update(sum);

      mark_inputs_consumed();
    }
  }

  [[nodiscard]] static std::unique_ptr<AddNode> create(const size_t num_inputs = 2) {
    auto node = std::make_unique<AddNode>(num_inputs);
    node->add_output("sum");
    return std::move(node);
  }
};

/**
 * Subtracts the second input stream from the first and writes the result to the output stream.
 */
struct SubtractNode : MultiInputNode {
  explicit SubtractNode() : MultiInputNode(2) {
    type = Subtract;
    name = "Subtract";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(in_a->value - in_b->value);
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<SubtractNode> create() {
    auto node = std::make_unique<SubtractNode>();
    node->add_output("a-b");
    return std::move(node);
  }
};

/**
 * Computes the remainder of dividing the first input stream by the second and writes the result to the output stream.
 */
struct ModuloNode : MultiInputNode {
  explicit ModuloNode() : MultiInputNode(2) {
    type = Modulo;
    name = "Modulo";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in_a || !in_b || !out) {
      return;
    }

    out->update(std::fmod(in_a->value, in_b->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<ModuloNode> create() {
    auto node = std::make_unique<ModuloNode>();
    node->add_output("a%b");
    return std::move(node);
  }
};

/**
 * Raises the first input stream to the power of the second and writes the result to the output stream.
 */
struct PowerNode : MultiInputNode {
  explicit PowerNode() : MultiInputNode(2) {
    type = Power;
    name = "Power";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const auto *base = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    const auto *exponent = dynamic_cast<Stream<float> *>(inputs[1].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!base || !exponent || !out) {
      return;
    }

    out->update(std::pow(base->value, exponent->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<PowerNode> create() {
    auto node = std::make_unique<PowerNode>();
    node->add_output("a^b");
    return std::move(node);
  }
};

/**
 * Selects the smaller of two input streams and writes the result to the output stream.
 */
struct MinNode : MultiInputNode {
  explicit MinNode(const uint8_t num_inputs = 2) : MultiInputNode(num_inputs) {
    type = Min;
    name = "Min";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    auto *const out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!out) {
      return;
    }

    float min = std::numeric_limits<float>::max();

    for (const auto &pin: inputs) {
      // This if avoids crashes on disconnected pins
      if (const auto *const in = dynamic_cast<Stream<float> *>(pin.stream.get())) {
        min = std::min(min, in->value);
      }
    }

    out->update(min);
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<MinNode> create(const uint8_t num_inputs = 2) {
    auto node = std::make_unique<MinNode>(num_inputs);
    node->add_output("min");
    return std::move(node);
  }
};

/**
 * Selects the larger of two input streams and writes the result to the output stream.
 */
struct MaxNode : MultiInputNode {
  explicit MaxNode(const uint8_t num_inputs = 2) : MultiInputNode(num_inputs) {
    type = Max;
    name = "Max";
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    auto *const out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!out) {
      return;
    }

    float max = std::numeric_limits<float>::min();

    for (const auto &pin: inputs) {
      // This if avoids crashes on disconnected pins
      if (const auto *const in = dynamic_cast<Stream<float> *>(pin.stream.get())) {
        max = std::max(max, in->value);
      }
    }

    out->update(max);
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<MaxNode> create(const uint8_t num_inputs = 2) {
    auto node = std::make_unique<MaxNode>(num_inputs);
    node->add_output("max");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    out->update(std::abs(in->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<AbsNode> create() {
    auto node = std::make_unique<AbsNode>();
    node->add_input("a");
    node->add_output("abs(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    out->update(std::floor(in->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<FloorNode> create() {
    auto node = std::make_unique<FloorNode>();
    node->add_input("a");
    node->add_output("floor(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    out->update(std::ceil(in->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<CeilNode> create() {
    auto node = std::make_unique<CeilNode>();
    node->add_input("a");
    node->add_output("ceil(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    out->update(std::round(in->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<RoundNode> create() {
    auto node = std::make_unique<RoundNode>();
    node->add_input("a");
    node->add_output("round(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    // Clamp to avoid NaN from negative values
    const float value = std::max(0.0f, in->value);
    out->update(std::sqrt(value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<SqrtNode> create() {
    auto node = std::make_unique<SqrtNode>();
    node->add_input("a");
    node->add_output("sqrt(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    out->update(-in->value);
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<NegateNode> create() {
    auto node = std::make_unique<NegateNode>();
    node->add_input("a");
    node->add_output("-a");
    return std::move(node);
  }
};

/**
 *  Computes the sine of the input stream and writes the result to the output stream.
 */
struct SinNode : Node {
  explicit SinNode() {
    type = Sin;
    name = "Sin";
  }

  void evaluate() override {
    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      throw std::runtime_error("invalid connections");
    }

    out->update(std::sin(in->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<SinNode> create() {
    auto node = std::make_unique<SinNode>();
    node->add_input("a");
    node->add_output("sin(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    out->update(std::cos(in->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<CosNode> create() {
    auto node = std::make_unique<CosNode>();
    node->add_input("a");
    node->add_output("cos(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    out->update(std::tan(in->value));
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<TanNode> create() {
    auto node = std::make_unique<TanNode>();
    node->add_input("a");
    node->add_output("tan(a)");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

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

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["in_min"] = in_min;
    j["in_max"] = in_max;
    j["out_min"] = out_min;
    j["out_max"] = out_max;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (j.contains("in_min")) in_min = j["in_min"];
      if (j.contains("in_max")) in_max = j["in_max"];
      if (j.contains("out_min")) out_min = j["out_min"];
      if (j.contains("out_max")) out_max = j["out_max"];
      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize RemapNode params: ") + e.what()
      );
    }
  }

  [[nodiscard]] static std::unique_ptr<RemapNode> create(const float in_min = 0.0f,
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
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in || !out) {
      return;
    }

    const float clamped = std::clamp(in->value, min_value, max_value);
    out->update(clamped);
    mark_inputs_consumed();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["min_value"] = min_value;
    j["max_value"] = max_value;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (j.contains("min_value")) min_value = j["min_value"];
      if (j.contains("max_value")) max_value = j["max_value"];
      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize Clamp params: ") + e.what()
      );
    }
  }

  [[nodiscard]] static std::unique_ptr<ClampNode> create(const float min_value = 0.0f,
                                                         const float max_value = 1.0f) {
    auto node = std::make_unique<ClampNode>();
    node->min_value = min_value;
    node->max_value = max_value;
    node->add_input("in");
    node->add_output("out");
    return std::move(node);
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

    const auto *in_a = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    const auto *in_b = dynamic_cast<Stream<float> *>(inputs[1].stream.get());
    const auto *in_t = dynamic_cast<Stream<float> *>(inputs[2].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

    if (!in_a || !in_b || !in_t || !out) {
      return;
    }

    const float t = std::clamp(in_t->value, 0.0f, 1.0f);
    const float result = in_a->value + t * (in_b->value - in_a->value);

    out->update(result);
    mark_inputs_consumed();
  }

  [[nodiscard]] static std::unique_ptr<LerpNode> create() {
    auto node = std::make_unique<LerpNode>();
    node->add_input("a");
    node->add_input("b");
    node->add_input("t");
    node->add_output("out");
    return std::move(node);
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

    const auto *in = dynamic_cast<Stream<float> *>(inputs[0].stream.get());
    auto *out = dynamic_cast<Stream<float> *>(outputs[0].stream.get());

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

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["edge0"] = edge0;
    j["edge1"] = edge1;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (j.contains("edge0")) edge0 = j["edge0"];
      if (j.contains("edge1")) edge1 = j["edge1"];
      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize SmoothStep params: ") + e.what()
      );
    }
  }

  [[nodiscard]] static std::unique_ptr<SmoothStepNode> create(const float edge0 = 0.0f,
                                                              const float edge1 = 1.0f) {
    auto node = std::make_unique<SmoothStepNode>();
    node->edge0 = edge0;
    node->edge1 = edge1;
    node->add_input("in");
    node->add_output("out");
    return std::move(node);
  }
};


#endif //NAG_ENGINE_MATH_NODES_H
