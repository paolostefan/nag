#ifndef NAG_ENGINE_PARTICLE_EMITTER_NODE_H
#define NAG_ENGINE_PARTICLE_EMITTER_NODE_H

#include <random>

#include "engine/nodes/node.h"
#include "engine/particles2d.h"
#include "engine/property_widget.h"

struct ParticleEmitterNode : Node {
  float time{0.f};
  float rate{10.f};
  float speed{100.f};
  float min_life{0.5f};
  float max_life{2.0f};
  int max_particles{10000};
  int seed{42};

  float spawn_accumulator{0.f};
  std::mt19937 rng;

  explicit ParticleEmitterNode(const int seed = 42) {
    type = NodeType::ParticleEmitter;
    name = "Particle Emitter";
    rng.seed(seed);
  }

  void step(const float dt) {
    time += dt;
    spawn_accumulator += rate * dt;

    // Spawn new particles
    while (spawn_accumulator >= 1.0f && particles_.particles.size() < static_cast<size_t>(max_particles)) {
      spawn_particle();
      spawn_accumulator -= 1.0f;
    }

    // Update existing particles
    for (auto &p : particles_.particles) {
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      p.life -= dt;
    }

    // Remove dead particles
    auto &vec = particles_.particles;
    std::erase_if(vec, [](const Particle2D &p) { return p.life <= 0.f; });

    evaluate();
  }

  void evaluate() override {
    if (!outputs.empty()) {
      if (const auto out = dynamic_cast<Stream<Particles2D> *>(outputs[0].stream.get())) {
        out->update(particles_);
      }
    }
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["rate"] = rate;
    j["speed"] = speed;
    j["min_life"] = min_life;
    j["max_life"] = max_life;
    j["max_particles"] = max_particles;
    j["seed"] = seed;
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

  [[nodiscard]] size_t particle_count() const { return particles_.particles.size(); }

  static std::unique_ptr<Node> create() {
    auto node = std::make_unique<ParticleEmitterNode>();
    node->add_typed_output<Particles2D>("particles");
    return node;
  }

private:
  Particles2D particles_;

  Particles2D &particles() { return particles_; }
  [[nodiscard]] const Particles2D &particles() const { return particles_; }

  void spawn_particle() {
    const float angle = std::uniform_real_distribution(0.f, 2.f * 3.14159265f)(rng);
    const float life = std::uniform_real_distribution(min_life, max_life)(rng);

    Particle2D p;
    p.vx = std::cos(angle) * speed;
    p.vy = std::sin(angle) * speed;
    p.life = life;
    p.max_life = life;
    particles_.particles.push_back(p);
  }
};

#endif // NAG_ENGINE_PARTICLE_EMITTER_NODE_H
