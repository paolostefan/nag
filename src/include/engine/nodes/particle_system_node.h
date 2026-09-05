#ifndef NAG_ENGINE_PARTICLE_SYSTEM_NODE_H
#define NAG_ENGINE_PARTICLE_SYSTEM_NODE_H

#include <cstring>

#include "engine/nodes/node.h"
#include "spdlog/spdlog.h"

struct ParticleSystemNode : Node {
  // Will be saved to the "pad" Pin member of added forces
  enum class ParticleSystemForce:uint8_t {
    Unknown = 0, // Used for string parse errors
    AccelerationX = 10, // Acceleration along X (e.g. wind)
    AccelerationY, // Acceleration along Y (e.g gravity)
    Radial, // Radial acceleration, proportional to the distance from the particle system center
    DragX, // Exponential damping along X
    DragY, // Exponential damping along Y
    Vortex, // Spiral acceleration
  };

  uint32_t population{0}; // Number of particles active
  uint32_t casualties{0}; // Number of particles who died this frame
  int rename_pin_id{-1}; // UI: Id of the force pin being renamed
  char pin_name_buf[127]{}; // UI: buffer for pin name
  bool is_renaming{false}; // UI: Are we renaming a force pin?

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Particle System"; }

  static std::string to_string(const ParticleSystemForce force) {
    switch (force) {
      case ParticleSystemForce::AccelerationX:
        return "AccelerationX";

      case ParticleSystemForce::AccelerationY:
        return "AccelerationY";

      case ParticleSystemForce::DragX:
        return "DragX";

      case ParticleSystemForce::DragY:
        return "DragY";

      case ParticleSystemForce::Radial:
        return "Radial";

      case ParticleSystemForce::Vortex:
        return "Vortex";

      default:
        return "Unknown";
    }
  }

  static ParticleSystemForce from_string(const std::string &force_string) {
    if (force_string == "AccelerationX") {
      return ParticleSystemForce::AccelerationX;
    }

    if (force_string == "AccelerationY") {
      return ParticleSystemForce::AccelerationY;
    }

    if (force_string == "DragX") {
      return ParticleSystemForce::DragX;
    }

    if (force_string == "DragY") {
      return ParticleSystemForce::DragY;
    }

    if (force_string == "Radial") {
      return ParticleSystemForce::Radial;
    }

    if (force_string == "Vortex") {
      return ParticleSystemForce::Vortex;
    }

    return ParticleSystemForce::Unknown;
  }

  explicit ParticleSystemNode() {
    type = NodeType::ParticleSystem;
    name = "Particle System";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    Particles2D *out_particles = outputs[0].get_particles();
    if (!out_particles) { return; }

    const Particles2D *new_particles = inputs[0].get_particles();
    if (!new_particles || !outputs[0].get_particles()) { return; }

    const float dt_ = new_particles->dt;

    // Add all the new particles to the output stream
    for (const auto &p: new_particles->particles) {
      out_particles->particles.push_back(p);
    }

    // Reset acceleration on all particles; decrease life
    for (auto &p: out_particles->particles) {
      p.ax = p.ay = 0.f;
      // Decrease life by dt
      p.life -= dt_;
    }

    const uint32_t pop_with_dead = out_particles->particles.size();

    // Remove dead particles
    std::erase_if(
      out_particles->particles,
      [](const Particle2D &p) { return p.life <= 0.f; }
    );

    population = out_particles->particles.size();
    casualties = pop_with_dead - population;

    // Update particle acceleration due to forces
    for (size_t i = 1; i < inputs.size(); ++i) {
      const float *val_p = inputs[i].get_float();

      if (!val_p) continue;

      const float val = *val_p;

      switch ((ParticleSystemForce) inputs[i].pad) {
        case ParticleSystemForce::AccelerationX:
          for (auto &p: out_particles->particles) {
            p.ax += val;
          }
          break;

        case ParticleSystemForce::AccelerationY:
          for (auto &p: out_particles->particles) {
            p.ay += val;
          }
          break;

        case ParticleSystemForce::Radial:
          for (auto &p: out_particles->particles) {
            const float dist = sqrtf(p.x * p.x + p.y * p.y);

            p.ax += -val * p.x / dist;
            p.ay += -val * p.y / dist;
          }
          break;

        case ParticleSystemForce::DragX:
          for (auto &p: out_particles->particles) {
            p.ax += -val * p.vx;
          }
          break;

        case ParticleSystemForce::DragY:
          for (auto &p: out_particles->particles) {
            p.ay += -val * p.vy;
          }
          break;

        case ParticleSystemForce::Vortex:
          for (auto &p: out_particles->particles) {
            const float dist = sqrtf(p.x * p.x + p.y * p.y);

            p.ax += -val * p.y / dist;
            p.ay += -val * p.x / dist;
          }
          break;
        default:
          break;
      }
    }

    // Update all particles' positions
    for (auto &p: out_particles->particles) {
      // Update velocity based on acceleration
      p.vx += p.ax * dt_;
      p.vy += p.ay * dt_;

      // Update position based on velocity
      p.x += p.vx * dt_;
      p.y += p.vy * dt_;
    }
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = nlohmann::json::object();

    j["force_slots"] = nlohmann::json::array();

    for (size_t i = 1; i < inputs.size(); i++) {
      nlohmann::json slot = nlohmann::json::object();
      slot["name"] = inputs[i].name;
      slot["type"] = to_string((ParticleSystemForce) (inputs[i].pad));

      j["force_slots"].push_back(slot);
    }

    return j;
  }

  OperationResult deserialize_params(const nlohmann::json &j) override {
    if (j.contains("force_slots")) {
      const nlohmann::json &forces_j = j["force_slots"];
      if (!forces_j.is_array()) {
        spdlog::warn("ParticleSystemNode::deserialize_params(): force_slots must be array, instead found {}",
                     forces_j.type_name());
        return OperationResult::error("wrong force_slots type");
      }

      for (const auto &it: forces_j) {
        const std::string force_type = it["type"];
        const ParticleSystemForce f = from_string(force_type);
        if (f == ParticleSystemForce::Unknown) {
          spdlog::warn("ParticleSystemNode::deserialize_params(): cannot parse force type '{}'",
                       force_type);
          continue;
        }

        if (!it["name"].is_string()) {
          spdlog::warn("ParticleSystemNode::deserialize_params(): force \"name\" must be string");
          continue;
        }

        Pin *force_pin = add_input(DataType::Float, it["name"]);
        force_pin->pad = static_cast<uint8_t>(f);
      }
    }
    return OperationResult::ok();
  }

  [[nodiscard]] float get_param(const std::string &/*param_name*/) const override {
    // No parameters for now, so always return 0
    return 0.f;
  }

  /// Factory method to create a new ParticleSystemNode with the correct input and output pins
  /// The input pin will be for the delta particle data, and the output pin will be for the processed particle data to be rendered
  static std::unique_ptr<Node> create() {
    auto node = std::make_unique<ParticleSystemNode>();

    node->add_input(DataType::Particles2D, "new_particles");
    node->add_output(DataType::Particles2D, "out_particles");

    return node;
  }
};


#endif //NAG_ENGINE_PARTICLE_SYSTEM_NODE_H
