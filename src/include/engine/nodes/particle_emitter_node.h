#ifndef NAG_ENGINE_PARTICLE_EMITTER_NODE_H
#define NAG_ENGINE_PARTICLE_EMITTER_NODE_H

#include <random>

#include "engine/nodes/node.h"
#include "engine/particles2d.h"
#include "engine/property_widget.h"

struct ParticleEmitterNode : Node {
  std::mt19937 rng;
  float    last_time{0.f};
  float    rate{10.f};
  float    speed{100.f};
  float    min_life{0.5f};
  float    max_life{2.f};
  float    spawn_accumulator{0.f};
  uint32_t max_particles{10000};
  int      seed{42};

  explicit ParticleEmitterNode(const int seed_ = 42) : seed(seed_) {
    type = NodeType::ParticleEmitter;
    name = "Particle Emitter";
    rng.seed(seed_);
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Particle Emitter"; }

  void evaluate() override {
    // Spawn particles based on the rate and elapsed time
    const float *in = inputs.empty() ? nullptr : inputs[0].get_float();
    if (!in || outputs.empty()) { return; }

    Particles2D particles_;
    particles_.dt = *in - last_time;
    last_time     = *in;

    // Accumulate spawn count based on rate and elapsed time
    spawn_accumulator += rate * particles_.dt;

    // Reserve space to avoid reallocations during particle spawning
    particles_.particles.reserve(static_cast<size_t>(spawn_accumulator) + 1);


    while (spawn_accumulator >= 1.f && particles_.particles.size() < max_particles) {
      particles_.particles.push_back(spawn_particle());
      spawn_accumulator -= 1.f;
    }

    outputs[0].set_particles(particles_);
    mark_inputs_consumed();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["rate"]          = rate;
    j["speed"]         = speed;
    j["min_life"]      = min_life;
    j["max_life"]      = max_life;
    j["max_particles"] = max_particles;
    j["seed"]          = seed;
    return j;
  }

  OperationResult deserialize_params(const nlohmann::json &j) override {
    if (j.contains("rate")) rate = j["rate"];
    if (j.contains("speed")) speed = j["speed"];
    if (j.contains("min_life")) min_life = j["min_life"];
    if (j.contains("max_life")) max_life = j["max_life"];
    if (j.contains("max_particles")) max_particles = j["max_particles"];
    if (j.contains("seed")) {
      seed = j["seed"];
      rng.seed(seed);
    }
    return OperationResult::ok();
  }

  void draw_properties(NodeGraph &graph, CommandHistory &history) override {
    PropertyWidget::SliderFloat(
      "Rate",
      id, rate,
      [](Node &n, const float v) { dynamic_cast<ParticleEmitterNode &>(n).rate = v; },
      graph, history,
      0.1f, 500.f, "%.1f"
    );
    PropertyWidget::SliderFloat(
      "Speed",
      id, speed,
      [](Node &n, const float v) { dynamic_cast<ParticleEmitterNode &>(n).speed = v; },
      graph, history,
      0.f, 1000.f, "%.1f"
    );
    PropertyWidget::SliderFloat(
      "Min Life",
      id, min_life,
      [](Node &n, const float v) { dynamic_cast<ParticleEmitterNode &>(n).min_life = v; },
      graph, history,
      0.1f, 10.f, "%.2f"
    );
    PropertyWidget::SliderFloat(
      "Max Life",
      id, max_life,
      [](Node &n, const float v) { dynamic_cast<ParticleEmitterNode &>(n).max_life = v; },
      graph, history,
      0.1f, 10.f, "%.2f"
    );
  }

  [[nodiscard]] float get_param(const std::string &param_name) const override {
    if (param_name == "rate") return rate;
    if (param_name == "speed") return speed;
    if (param_name == "min_life") return min_life;
    if (param_name == "max_life") return max_life;
    if (param_name == "max_particles") return static_cast<float>(max_particles);
    return 0.f;
  }

  /// Factory method to create a new ParticleEmitterNode with the correct output pin
  static std::unique_ptr<Node> create() {
    auto node = std::make_unique<ParticleEmitterNode>();
    node->add_input(DataType::Float, "time");
    node->add_output(DataType::Particles2D, "particles");
    return node;
  }

private:
  Particle2D spawn_particle() {
    const float angle = std::uniform_real_distribution(0.f, 2.f * 3.14159265f)(rng);
    const float life  = std::uniform_real_distribution(min_life, max_life)(rng);

    Particle2D p;
    p.vx       = std::cos(angle) * speed;
    p.vy       = std::sin(angle) * speed;
    p.life     = life;
    p.max_life = life;
    return p;
  }
};

#endif // NAG_ENGINE_PARTICLE_EMITTER_NODE_H
