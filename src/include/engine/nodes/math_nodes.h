#ifndef NAG_ENGINE_MATH_NODES_H
#define NAG_ENGINE_MATH_NODES_H

#include <algorithm>
#include <cmath>

#include "engine/nodes/node.h"

// ===========================================================================
// BINARY OPERATORS (2 inputs -> 1 output)
// ===========================================================================

/**
 *  Multiplies two (or more) input streams and writes the result to the output stream.
 */
struct MultiplyNode : MultiInputNode {
  explicit MultiplyNode() : MultiInputNode(2) {
    type = NodeType::Multiply;
    name = "Multiply";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Multiply"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    float result = 1.f;
    for (const auto &pin: inputs) {
      if (const float *in = pin.get_float()) {
        result *= *in;
      } // invalid pins are ignored
    }

    outputs[0].set_float(result);
  }

  [[nodiscard]] static std::unique_ptr<MultiplyNode> create() {
    auto node = std::make_unique<MultiplyNode>();
    node->add_output(DataType::Float, "product");
    return node;
  }
};

/**
 * Divides two input streams and writes the result to the output stream.
 * Avoids division by zero by clamping the divisor to a small epsilon value.
 */
struct DivideNode : MultiInputNode {
  static constexpr float kEpsilon{1e-6f}; // Avoid division by zero

  explicit DivideNode() : MultiInputNode(2) {
    type = NodeType::Divide;
    name = "Divide";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Divide"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const float *in_a = inputs[0].get_float();
    const float *in_b = inputs[1].get_float();
    if (!in_a || !in_b || !outputs[0].get_float()) { return; }
    float divisor = *in_b;
    if (std::abs(divisor) < kEpsilon) { divisor = kEpsilon; }
    outputs[0].set_float((*in_a) / divisor);
  }

  [[nodiscard]] static std::unique_ptr<DivideNode> create() {
    auto node = std::make_unique<DivideNode>();
    node->add_output(DataType::Float, "quotient");
    return node;
  }
};

/**
 * Adds two input streams and writes the result to the output stream.
 */
struct AddNode : MultiInputNode {
  explicit AddNode(const size_t num_inputs = 2) : MultiInputNode(num_inputs) {
    type = NodeType::Add;
    name = "Add";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Add"; }

  /**
   * Sum all input streams and write the result to the output stream.
   */
  void evaluate() override {
    if (!outputs.empty()) {
      if (!outputs[0].get_float()) { return; }
      float sum = 0.f;
      for (const auto &pin: inputs) {
        if (const float *in = pin.get_float()) {
          sum += *in;
        }
      }
      outputs[0].set_float(sum);
    }
  }

  [[nodiscard]] static std::unique_ptr<AddNode> create(const size_t num_inputs = 2) {
    auto node = std::make_unique<AddNode>(num_inputs);
    node->add_output(DataType::Float, "sum");
    return node;
  }
};

/**
 * Subtracts the second input stream from the first and writes the result to the output stream.
 */
struct SubtractNode : MultiInputNode {
  explicit SubtractNode() : MultiInputNode(2) {
    type = NodeType::Subtract;
    name = "Subtract";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Subtract"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    if (!inputs.empty()) {
      const size_t inputs_ct = inputs.size();
      const auto *in0 = inputs[0].get_float();
      float result = in0 ? *in0 : 0.f;

      for (size_t i = 1; i < inputs_ct; i++) {
        if (const float *in = inputs[i].get_float()) {
          result -= *in;
        } // invalid pins are ignored
      }

      outputs[0].set_float(result);
    }
  }

  [[nodiscard]] static std::unique_ptr<SubtractNode> create() {
    auto node = std::make_unique<SubtractNode>();
    node->add_output(DataType::Float, "a-b");
    return node;
  }
};

/**
 * Computes the remainder of dividing the first input stream by the second and writes the result to the output stream.
 */
struct ModuloNode : MultiInputNode {
  explicit ModuloNode() : MultiInputNode(2) {
    type = NodeType::Modulo;
    name = "Modulo";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Modulo"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const float *in_a = inputs[0].get_float();
    const float *in_b = inputs[1].get_float();
    if (!in_a || !in_b || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::fmod(*in_a, *in_b));
  }

  [[nodiscard]] static std::unique_ptr<ModuloNode> create() {
    auto node = std::make_unique<ModuloNode>();
    node->add_output(DataType::Float, "a%b");
    return node;
  }
};

/**
 * Raises the first input stream to the power of the second and writes the result to the output stream.
 */
struct PowerNode : MultiInputNode {
  explicit PowerNode() : MultiInputNode(2) {
    type = NodeType::Power;
    name = "Power";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Power"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const float *base = inputs[0].get_float();
    const float *exponent = inputs[1].get_float();
    if (!base || !exponent || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::pow(*base, *exponent));
  }

  [[nodiscard]] static std::unique_ptr<PowerNode> create() {
    auto node = std::make_unique<PowerNode>();
    node->add_output(DataType::Float, "a^b");
    return node;
  }
};

/**
 * Selects the smaller of two input streams and writes the result to the output stream.
 */
struct MinNode : MultiInputNode {
  explicit MinNode(const uint8_t num_inputs = 2) : MultiInputNode(num_inputs) {
    type = NodeType::Min;
    name = "Min";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Min"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    if (!outputs[0].get_float()) { return; }
    float min = std::numeric_limits<float>::max();

    for (const auto &pin: inputs) {
      // This if avoids crashes on disconnected pins
      if (const float *in = pin.get_float()) {
        min = std::min(min, *in);
      }
    }

    outputs[0].set_float(min);
  }

  [[nodiscard]] static std::unique_ptr<MinNode> create(const uint8_t num_inputs = 2) {
    auto node = std::make_unique<MinNode>(num_inputs);
    node->add_output(DataType::Float, "min");
    return node;
  }
};

/**
 * Selects the larger of two input streams and writes the result to the output stream.
 */
struct MaxNode : MultiInputNode {
  explicit MaxNode(const uint8_t num_inputs = 2) : MultiInputNode(num_inputs) {
    type = NodeType::Max;
    name = "Max";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Max"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    if (!outputs[0].get_float()) { return; }
    float max = std::numeric_limits<float>::min();

    for (const auto &pin: inputs) {
      // This if avoids crashes on disconnected pins
      if (const float *in = pin.get_float()) {
        max = std::max(max, *in);
      }
    }

    outputs[0].set_float(max);
  }

  [[nodiscard]] static std::unique_ptr<MaxNode> create(const uint8_t num_inputs = 2) {
    auto node = std::make_unique<MaxNode>(num_inputs);
    node->add_output(DataType::Float, "max");
    return node;
  }
};

/**
 * Compares two input streams and writes the result of "a > b" to the output stream as a boolean.
 */
struct CompareNode : MultiInputNode {
  explicit CompareNode() : MultiInputNode(2) {
    type = NodeType::Compare;
    name = "Compare";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Compare"; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const float *in_a = inputs[0].get_float();
    const float *in_b = inputs[1].get_float();
    if (!in_a || !in_b || !outputs[0].get_bool()) { return; }
    outputs[0].set_bool((*in_a) > (*in_b));
  }

  [[nodiscard]] static std::unique_ptr<CompareNode> create() {
    auto node = std::make_unique<CompareNode>();
    node->add_output(DataType::Bool, "a>b");
    return node;
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
    type = NodeType::Abs;
    name = "Abs";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Abs"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::abs(*in));
  }

  [[nodiscard]] static std::unique_ptr<AbsNode> create() {
    auto node = std::make_unique<AbsNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "abs(a)");
    return node;
  }
};

/**
 * Rounds down to the nearest integer.
 */
struct FloorNode : Node {
  FloorNode() {
    type = NodeType::Floor;
    name = "Floor";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Floor"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::floor(*in));
  }

  [[nodiscard]] static std::unique_ptr<FloorNode> create() {
    auto node = std::make_unique<FloorNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "floor(a)");
    return node;
  }
};

/**
 * Rounds up to the nearest integer.
 */
struct CeilNode : Node {
  CeilNode() {
    type = NodeType::Ceil;
    name = "Ceil";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Ceil"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::ceil(*in));
  }

  [[nodiscard]] static std::unique_ptr<CeilNode> create() {
    auto node = std::make_unique<CeilNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "ceil(a)");
    return node;
  }
};

/**
 * Rounds to the nearest integer.
 */
struct RoundNode : Node {
  RoundNode() {
    type = NodeType::Round;
    name = "Round";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Round"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::round(*in));
  }

  [[nodiscard]] static std::unique_ptr<RoundNode> create() {
    auto node = std::make_unique<RoundNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "round(a)");
    return node;
  }
};

/**
 *  Computes the square root of the input stream and writes the result to the output stream.
 *  Avoids NaN by clamping negative values to zero.
 */
struct SqrtNode : Node {
  SqrtNode() {
    type = NodeType::Sqrt;
    name = "Sqrt";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Square root"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    const float value = std::max(0.f, *in);
    outputs[0].set_float(std::sqrt(value));
  }

  [[nodiscard]] static std::unique_ptr<SqrtNode> create() {
    auto node = std::make_unique<SqrtNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "sqrt(a)");
    return node;
  }
};

/**
 * Negates the input stream and writes the result to the output stream.
 */
struct NegateNode : Node {
  NegateNode() {
    type = NodeType::Negate;
    name = "Negate";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Negate"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(-(*in));
  }

  [[nodiscard]] static std::unique_ptr<NegateNode> create() {
    auto node = std::make_unique<NegateNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "-a");
    return node;
  }
};

/**
 *  Computes the sine of the input stream and writes the result to the output stream.
 */
struct SinNode : Node {
  explicit SinNode() {
    type = NodeType::Sin;
    name = "Sin";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Sin"; }

  void evaluate() override {
    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::sin(*in));
  }

  [[nodiscard]] static std::unique_ptr<SinNode> create() {
    auto node = std::make_unique<SinNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "sin(a)");
    return node;
  }
};


/**
 * Computes the cosine of the input stream and writes the result to the output stream.
 */
struct CosNode : Node {
  CosNode() {
    type = NodeType::Cos;
    name = "Cos";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Cos"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::cos(*in));
  }

  [[nodiscard]] static std::unique_ptr<CosNode> create() {
    auto node = std::make_unique<CosNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "cos(a)");
    return node;
  }
};

/**
 * Computes the tangent of the input stream and writes the result to the output stream.
 */
struct TanNode : Node {
  TanNode() {
    type = NodeType::Tan;
    name = "Tan";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Tan"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }
    outputs[0].set_float(std::tan(*in));
  }

  [[nodiscard]] static std::unique_ptr<TanNode> create() {
    auto node = std::make_unique<TanNode>();
    node->add_input(DataType::Float, "a");
    node->add_output(DataType::Float, "tan(a)");
    return node;
  }
};

// ===========================================================================
// SPECIAL OPERATORS
// ===========================================================================

/**
 * Maps value from [in_min, in_max] to [out_min, out_max].
 */
struct RemapNode : Node {
  float in_min{0.f};
  float in_max{1.f};
  float out_min{0.f};
  float out_max{1.f};
  std::vector<Property> props_;

  RemapNode() {
    type = NodeType::Remap;
    name = "Remap";
    props_ = {
      MakeFloatProp(*this, &RemapNode::in_min, "in_min", "In Min",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
      MakeFloatProp(*this, &RemapNode::in_max, "in_max", "In Max",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
      MakeFloatProp(*this, &RemapNode::out_min, "out_min", "Out Min",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
      MakeFloatProp(*this, &RemapNode::out_max, "out_max", "Out Max",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
    };
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Remap"; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }

    // Avoid division by zero
    const float in_range = in_max - in_min;
    if (std::abs(in_range) < 1e-6f) {
      outputs[0].set_float(out_min);
      return;
    }

    // Linear mapping
    const float t = (*in - in_min) / in_range;
    const float result = out_min + t * (out_max - out_min);

    outputs[0].set_float(result);
  }

  [[nodiscard]] static std::unique_ptr<RemapNode> create(const float in_min = 0.f,
                                                         const float in_max = 1.f,
                                                         const float out_min = 0.f,
                                                         const float out_max = 1.f) {
    auto node = std::make_unique<RemapNode>();
    node->in_min = in_min;
    node->in_max = in_max;
    node->out_min = out_min;
    node->out_max = out_max;
    node->add_input(DataType::Float, "in");
    node->add_output(DataType::Float, "out");
    return node;
  }
};

/**
 * Clamps the input stream to the specified range and writes the result to the output stream.
 */
struct ClampNode : Node {
  float min_value{0.f};
  float max_value{1.f};
  std::vector<Property> props_;

  ClampNode() {
    type = NodeType::Clamp;
    name = "Clamp";
    props_ = {
      MakeFloatProp(*this, &ClampNode::min_value, "min_value", "Min",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
      MakeFloatProp(*this, &ClampNode::max_value, "max_value", "Max",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
    };
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Clamp"; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }

    const float clamped = std::clamp(*in, min_value, max_value);
    outputs[0].set_float(clamped);
  }

  [[nodiscard]] static std::unique_ptr<ClampNode> create(const float min_value = 0.f,
                                                         const float max_value = 1.f) {
    auto node = std::make_unique<ClampNode>();
    node->min_value = min_value;
    node->max_value = max_value;
    node->add_input(DataType::Float, "in");
    node->add_output(DataType::Float, "out");
    return node;
  }
};

/**
 * Interpolates between two input streams (A and B) based on a third input stream (0-1).
 */
struct LerpNode : Node {
  LerpNode() {
    type = NodeType::Lerp;
    name = "Lerp";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Lerp"; }

  void evaluate() override {
    if (inputs.size() < 3 || outputs.empty()) {
      return;
    }

    const float *in_a = inputs[0].get_float();
    const float *in_b = inputs[1].get_float();
    const float *in_t = inputs[2].get_float();
    if (!in_a || !in_b || !in_t || !outputs[0].get_float()) {
      return;
    }

    const float t = std::clamp(*in_t, 0.f, 1.f);
    const float result = *in_a + t * (*in_b - *in_a);

    outputs[0].set_float(result);
  }

  [[nodiscard]] static std::unique_ptr<LerpNode> create() {
    auto node = std::make_unique<LerpNode>();
    node->add_input(DataType::Float, "a");
    node->add_input(DataType::Float, "b");
    node->add_input(DataType::Float, "t");
    node->add_output(DataType::Float, "out");
    return node;
  }
};

/**
 * Smooth interpolation with ease in/out (Hermite interpolation)
 */
struct SmoothStepNode : Node {
  float edge0{0.f};
  float edge1{1.f};

  SmoothStepNode() {
    type = NodeType::SmoothStep;
    name = "SmoothStep";
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Smooth Step"; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) { return; }

    // Clamp t to [0, 1]
    const float t = std::clamp((*in - edge0) / (edge1 - edge0), 0.f, 1.f);

    // Hermite interpolation: 3t² - 2t³
    const float smooth = t * t * (3.f - 2.f * t);
    outputs[0].set_float(smooth);
  }

  [[nodiscard]] static std::unique_ptr<SmoothStepNode> create(const float edge0 = 0.f,
                                                              const float edge1 = 1.f) {
    auto node = std::make_unique<SmoothStepNode>();
    node->edge0 = edge0;
    node->edge1 = edge1;
    node->add_input(DataType::Float, "in");
    node->add_output(DataType::Float, "out");
    return node;
  }
};


#endif //NAG_ENGINE_MATH_NODES_H
