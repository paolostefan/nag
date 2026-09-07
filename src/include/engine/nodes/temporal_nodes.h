#ifndef NAG_ENGINE_TEMPORAL_NODES_H
#define NAG_ENGINE_TEMPORAL_NODES_H

#include <cmath>
#include <deque>
#include <memory>

#include "engine/nodes/node.h"

static constexpr float PI = M_PI;

/**
 * Low Frequency Oscillator.
 * Generates periodic waveforms based on input time.
 * Supports: sine, square, triangle, sawtooth waves.
 */
struct LFONode : Node {
  /**
   * Wave shape types for LFO.
   */
  enum class WaveShape : uint8_t {
    Sine,
    Square,
    Triangle,
    Sawtooth,
  };

  WaveShape wave_shape{WaveShape::Sine};
  float frequency{1.f}; // Hz
  float amplitude{1.f};
  float phase{0.f}; // Radians
  float offset{0.f}; // DC offset
  float pulse_width{0.5f}; // For square wave (0-1)
  std::vector<Property> props_;

  LFONode() {
    type = NodeType::LFO;
    name = "LFO";
    static constexpr const char *wave_shapes[] = {"Sine", "Square", "Triangle", "Sawtooth"};
    props_ = {
      MakeFloatProp(*this, &LFONode::frequency, "frequency", "Frequency",
                    WidgetKind::SliderFloat, 0.1f, 10.f, "%.2f"),
      MakeFloatProp(*this, &LFONode::amplitude, "amplitude", "Amplitude",
                    WidgetKind::SliderFloat, 0.f, 2.f, "%.2f"),
      MakeEnumProp(*this, &LFONode::wave_shape, "wave_shape", "Wave Shape",
                   wave_shapes, 4),
      MakeFloatProp(*this, &LFONode::phase, "phase", "Phase",
                    WidgetKind::SliderFloat, 0.f, 6.28f, "%.2f"),
      MakeFloatProp(*this, &LFONode::offset, "offset", "Offset",
                    WidgetKind::SliderFloat, -1.f, 1.f, "%.2f"),
      MakeFloatProp(*this, &LFONode::pulse_width, "pulse_width", "Pulse Width",
                    WidgetKind::SliderFloat, 0.01f, 0.99f, "%.2f"),
    };
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "LFO"; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    const float *in = inputs[0].get_float();
    if (!in || !outputs[0].get_float()) {
      return;
    }

    const float time = *in;
    const float angle = 2.f * PI * frequency * time + phase;

    float wave_value = 0.f;

    switch (wave_shape) {
      case WaveShape::Sine:
        wave_value = std::sin(angle);
        break;

      case WaveShape::Square: {
        const float normalized_phase = std::fmod(angle / (2.f * PI), 1.f);
        wave_value = normalized_phase < pulse_width ? 1.f : -1.f;
      }
      break;

      case WaveShape::Triangle: {
        const float normalized_phase = std::fmod(angle / (2.f * PI), 1.f);
        wave_value = 4.f * std::abs(normalized_phase - 0.5f) - 1.f;
      }
      break;

      case WaveShape::Sawtooth: {
        const float normalized_phase = std::fmod(angle / (2.f * PI), 1.f);
        wave_value = 2.f * normalized_phase - 1.f;
      }
      break;
    }

    const float result = offset + amplitude * wave_value;
    outputs[0].set_float(result);
  }

  /**
   * Create an LFO node with specified parameters.
   * @param frequency Oscillation frequency in Hz
   * @param amplitude Wave amplitude
   * @param wave_shape Wave shape (default: Sine)
   * @param phase Phase offset in radians (default: 0)
   * @param offset DC offset (default: 0)
   */
  static std::unique_ptr<LFONode> create(
    const float frequency = 1.f,
    const float amplitude = 1.f,
    const WaveShape wave_shape = WaveShape::Sine,
    const float phase = 0.f,
    const float offset = 0.f) {
    auto node = std::make_unique<LFONode>();
    node->frequency = frequency;
    node->amplitude = amplitude;
    node->wave_shape = wave_shape;
    node->phase = phase;
    node->offset = offset;
    node->add_input(DataType::Float, "time");
    node->add_output(DataType::Float, "wave");
    return node;
  }
};

/**
 * ADSR Envelope Generator.
 * Generates Attack-Decay-Sustain-Release envelope based on trigger input.
 * Trigger transitions: 0->1 starts attack, 1->0 starts release.
 */
struct EnvelopeNode : Node {
  /**
   * Envelope states.
   */
  enum State : uint8_t {
    Idle,
    Attack,
    Decay,
    Sustain,
    Release,
  };

  float attack_time{0.1f}; // Seconds
  float decay_time{0.1f}; // Seconds
  float sustain_level{0.7f}; // 0-1
  float release_time{0.2f}; // Seconds

  State current_state{Idle};
  float envelope_value{0.f};
  float state_start_time{0.f};
  float state_start_value{0.f};
  bool was_triggered{false};
  std::vector<Property> props_;

  EnvelopeNode() {
    type = NodeType::Envelope;
    name = "Envelope";
    props_ = {
      MakeFloatProp(*this, &EnvelopeNode::attack_time, "attack_time", "Attack",
                    WidgetKind::SliderFloat, 0.01f, 2.f, "%.2f"),
      MakeFloatProp(*this, &EnvelopeNode::decay_time, "decay_time", "Decay",
                    WidgetKind::SliderFloat, 0.01f, 2.f, "%.2f"),
      MakeFloatProp(*this, &EnvelopeNode::sustain_level, "sustain_level", "Sustain",
                    WidgetKind::SliderFloat, 0.f, 1.f, "%.2f"),
      MakeFloatProp(*this, &EnvelopeNode::release_time, "release_time", "Release",
                    WidgetKind::SliderFloat, 0.01f, 2.f, "%.2f"),
    };
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Envelope"; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const float *trigger_stream = inputs[0].get_float();
    const float *time_stream = inputs[1].get_float();
    if (!trigger_stream || !time_stream || !outputs[0].get_float()) {
      return;
    }

    const float trigger = *trigger_stream;
    const float time = *time_stream;
    const bool is_triggered = trigger > 0.5f;

    // Detect trigger transitions
    if (is_triggered && !was_triggered) {
      // Rising edge: start attack
      start_attack(time);
    } else if (!is_triggered && was_triggered) {
      // Falling edge: start release
      start_release(time);
    }

    was_triggered = is_triggered;

    // Update envelope based on current state
    update_envelope(time);

    outputs[0].set_float(envelope_value);
  }

private:
  /**
   * Start attack phase.
   */
  void start_attack(const float time) {
    current_state = Attack;
    state_start_time = time;
    state_start_value = envelope_value;
  }

  /**
   * Start release phase.
   */
  void start_release(const float time) {
    current_state = Release;
    state_start_time = time;
    state_start_value = envelope_value;
  }

  /**
   * Update envelope value based on current state and time.
   */
  void update_envelope(const float time) {
    const float elapsed = time - state_start_time;

    switch (current_state) {
      case Idle:
        envelope_value = 0.f;
        break;

      case Attack:
        if (attack_time > 0.f) {
          const float progress = std::min(1.f, elapsed / attack_time);
          envelope_value = state_start_value + progress * (1.f - state_start_value);

          if (progress >= 1.f) {
            current_state = Decay;
            state_start_time = time;
            state_start_value = 1.f;
          }
        } else {
          envelope_value = 1.f;
          current_state = Decay;
          state_start_time = time;
          state_start_value = 1.f;
        }
        break;

      case Decay:
        if (decay_time > 0.f) {
          const float progress = std::min(1.f, elapsed / decay_time);
          envelope_value = 1.f - progress * (1.f - sustain_level);

          if (progress >= 1.f) {
            current_state = Sustain;
          }
        } else {
          envelope_value = sustain_level;
          current_state = Sustain;
        }
        break;

      case Sustain:
        envelope_value = sustain_level;
        break;

      case Release:
        if (release_time > 0.f) {
          const float progress = std::min(1.f, elapsed / release_time);
          envelope_value = state_start_value * (1.f - progress);

          if (progress >= 1.f) {
            current_state = Idle;
            envelope_value = 0.f;
          }
        } else {
          envelope_value = 0.f;
          current_state = Idle;
        }
        break;
    }
  }

public:
  /**
   * Create an ADSR envelope node with specified timings.
   * @param attack_time Attack time in seconds
   * @param decay_time Decay time in seconds
   * @param sustain_level Sustain level (0-1)
   * @param release_time Release time in seconds
   */
  static std::unique_ptr<EnvelopeNode> create(
    const float attack_time = 0.1f,
    const float decay_time = 0.1f,
    const float sustain_level = 0.7f,
    const float release_time = 0.2f) {
    auto node = std::make_unique<EnvelopeNode>();
    node->attack_time = attack_time;
    node->decay_time = decay_time;
    node->sustain_level = sustain_level;
    node->release_time = release_time;
    node->add_input(DataType::Float, "trigger");
    node->add_input(DataType::Float, "time");
    node->add_output(DataType::Float, "envelope");
    return node;
  }
};

/**
 * Delay/Buffer Node.
 * Delays input signal by specified time using circular buffer.
 * Useful for echo effects and temporal offsets.
 */
struct DelayNode : Node {
  float delay_time{1.f}; // Seconds
  float sample_rate{60.f}; // Samples per second (matched to frame rate)

  std::deque<float> buffer;
  size_t max_buffer_size{0};
  float last_time{0.f};
  bool initialized{false};
  std::vector<Property> props_;

  DelayNode() {
    type = NodeType::Delay;
    name = "Delay";
    props_ = {
      MakeFloatProp(*this, &DelayNode::delay_time, "delay_time", "Delay Time",
                    WidgetKind::SliderFloat, 0.1f, 5.f, "%.2f"),
      MakeFloatProp(*this, &DelayNode::sample_rate, "sample_rate", "Sample Rate",
                    WidgetKind::SliderFloat, 10.f, 120.f, "%.0f"),
    };
    props_[0].set = [this](PropertyValue v) { delay_time = std::get<float>(v); update_buffer_size(); };
    props_[1].set = [this](PropertyValue v) { sample_rate = std::get<float>(v); update_buffer_size(); };
    update_buffer_size();
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Delay"; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  /**
   * Update buffer size based on delay time and sample rate.
   */
  void update_buffer_size() {
    max_buffer_size = static_cast<size_t>(delay_time * sample_rate);
    if (max_buffer_size < 1) {
      max_buffer_size = 1;
    }
  }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const float *value_stream = inputs[0].get_float();
    const float *time_stream = inputs[1].get_float();
    if (!value_stream || !time_stream || !outputs[0].get_float()) {
      return;
    }

    const float current_time = *time_stream;
    const float current_value = *value_stream;

    // Initialize on first run
    if (!initialized) {
      buffer.clear();
      buffer.resize(max_buffer_size, 0.f);
      last_time = current_time;
      initialized = true;
    }

    // Check if we should add a new sample (time-based sampling)
    const float time_delta = current_time - last_time;
    if (time_delta >= (1.f / sample_rate)) {
      // Add new value to buffer
      buffer.push_back(current_value);

      // Remove oldest value if buffer is full
      if (buffer.size() > max_buffer_size) {
        buffer.pop_front();
      }

      last_time = current_time;
    }

    // Output oldest value from buffer
    const float delayed_value = buffer.empty() ? 0.f : buffer.front();
    outputs[0].set_float(delayed_value);
  }

  /**
   * Create a delay node with specified delay time.
   * @param delay_time Delay duration in seconds
   * @param sample_rate Sampling rate in Hz (default: 60)
   */
  static std::unique_ptr<DelayNode> create(
    const float delay_time = 1.f,
    const float sample_rate = 60.f) {
    auto node = std::make_unique<DelayNode>();
    node->delay_time = delay_time;
    node->sample_rate = sample_rate;
    node->update_buffer_size();
    node->add_input(DataType::Float, "value");
    node->add_input(DataType::Float, "time");
    node->add_output(DataType::Float, "delayed");
    return node;
  }
};

/**
 * Smoother Node.
 * Smoothly interpolates to target value using exponential smoothing.
 * Also known as "one-pole filter" or "RC lowpass filter".
 * Useful for: removing jitter, smooth camera movements, easing transitions.
 */
struct SmootherNode : Node {
  float smooth_time{0.1f}; // Time to reach ~63% of target (seconds)

  float last_time{0.f};
  float current_value{0.f};
  bool initialized{false};
  std::vector<Property> props_;

  SmootherNode() {
    type = NodeType::Smoother;
    name = "Smoother";
    props_ = {
      MakeFloatProp(*this, &SmootherNode::smooth_time, "smooth_time", "Smooth Time",
                    WidgetKind::SliderFloat, 0.01f, 1.f, "%.2f"),
    };
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Smoother"; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

  void evaluate() override {
    if (inputs.size() < 2 || outputs.empty()) {
      return;
    }

    const float *target_stream = inputs[0].get_float();
    const float *time_stream = inputs[1].get_float();
    if (!target_stream || !time_stream || !outputs[0].get_float()) {
      return;
    }

    const float target = *target_stream;
    const float time = *time_stream;

    // Initialize on first run
    if (!initialized) {
      current_value = target;
      last_time = time;
      initialized = true;
    }

    const float dt = time - last_time;
    last_time = time;

    // Exponential smoothing
    // Formula: current += (target - current) * (1 - exp(-dt / smooth_time))
    if (smooth_time > 0.f && dt > 0.f) {
      const float alpha = 1.f - std::exp(-dt / smooth_time);
      current_value += (target - current_value) * alpha;
    } else {
      // No smoothing
      current_value = target;
    }

    outputs[0].set_float(current_value);
  }

  /**
   * Create a smoother node with specified smooth time.
   * @param smooth_time Time constant in seconds (lower = faster response)
   */
  static std::unique_ptr<SmootherNode> create(const float smooth_time = 0.1f) {
    auto node = std::make_unique<SmootherNode>();
    node->smooth_time = smooth_time;
    node->add_input(DataType::Float, "target");
    node->add_input(DataType::Float, "time");
    node->add_output(DataType::Float, "smoothed");
    return node;
  }
};

#endif //NAG_ENGINE_TEMPORAL_NODES_H
