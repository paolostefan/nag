#ifndef NAG_ENGINE_PARTICLE_EMITTER_NODE_H
#define NAG_ENGINE_PARTICLE_EMITTER_NODE_H

#include <random>

#include "engine/nodes/node.h"
#include "engine/particles2d.h"

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
  std::vector<Property> props_;

  explicit ParticleEmitterNode(const int seed_ = 42) : seed(seed_) {
    type = NodeType::ParticleEmitter;
    name = "Particle Emitter";
    rng.seed(seed_);

    // max_particles is uint32_t; Property::value holds int*. Layout-identical on
    // all supported platforms (C++ guarantees same size/align for int/unsigned).
    static_assert(sizeof(int) == sizeof(uint32_t));
    static_assert(alignof(int) == alignof(uint32_t));

    props_ = {
      MakeFloatProp(*this, &ParticleEmitterNode::rate, "rate", "Rate",
                    WidgetKind::SliderFloat, 0.1f, 500.f, "%.1f"),
      MakeFloatProp(*this, &ParticleEmitterNode::speed, "speed", "Speed",
                    WidgetKind::SliderFloat, 0.f, 1000.f, "%.1f"),
      MakeFloatProp(*this, &ParticleEmitterNode::min_life, "min_life", "Min Life",
                    WidgetKind::SliderFloat, 0.1f, 10.f, "%.2f"),
      MakeFloatProp(*this, &ParticleEmitterNode::max_life, "max_life", "Max Life",
                    WidgetKind::SliderFloat, 0.1f, 10.f, "%.2f"),
    };
    Property max_p;
    max_p.name = "max_particles";
    max_p.label = "Max Particles";
    max_p.kind = WidgetKind::InputInt;
    max_p.param_type = ParamType::Int;
    max_p.value = reinterpret_cast<int *>(&max_particles);
    max_p.get = [this]() -> PropertyValue { return static_cast<int>(max_particles); };
    max_p.set = [this](PropertyValue v) { max_particles = static_cast<uint32_t>(std::get<int>(v)); };
    props_.push_back(std::move(max_p));

    Property seed_p;
    seed_p.name = "seed";
    seed_p.label = "Seed";
    seed_p.kind = WidgetKind::InputInt;
    seed_p.param_type = ParamType::Int;
    seed_p.value = &seed;
    seed_p.get = [this]() -> PropertyValue { return seed; };
    seed_p.set = [this](PropertyValue v) { seed = std::get<int>(v); rng.seed(seed); };
    props_.push_back(std::move(seed_p));
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Particle Emitter"; }

  [[nodiscard]] const std::vector<Property> &properties() const noexcept override { return props_; }

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
