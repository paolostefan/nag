#ifndef NAG_ENGINE_PARTICLE_SYSTEM_NODE_H
#define NAG_ENGINE_PARTICLE_SYSTEM_NODE_H

#include <IconsFontAwesome6.h>

#include "engine/nodes/node.h"

struct ParticleSystemNode : Node {
  // Will be saved to the "pad" Pin member of added forces
  enum class ParticleSystemForce:uint8_t {
    AccelerationX = 10, // Acceleration along Y (e.g. gravity)
    AccelerationY,      // Acceleration along X
    Radial,             // Radial acceleration, proportional to the distance from the particle system center
    DragX,              // Exponential damping along X
    DragY,              // Exponential damping along Y
    Vortex,             // Spiral acceleration
  };

  uint32_t population{0};   // Number of particles active
  uint32_t casualties{0};   // Number of particles who died this frame
  int rename_pin_id{-1};    // UI: Id of the force pin being renamed
  char pin_name_buf[127]{}; // UI: buffer for pin name
  bool is_renaming{false};  // UI: Are we renaming a force pin?

  explicit ParticleSystemNode() {
    type = NodeType::ParticleSystem;
    name = "Particle System";
  }

  void evaluate() override {
    if (inputs.empty() || outputs.empty()) {
      return;
    }

    mark_inputs_consumed();

    Particles2D *out_particles = outputs[0].get_particles();
    if (!out_particles) { return; }

    const Particles2D *new_particles = inputs[0].get_particles();
    if (!new_particles || !outputs[0].get_particles()) { return; }

    const float dt = new_particles->dt;

    // Add all the new particles to the output stream
    for (const auto &p: new_particles->particles) {
      out_particles->particles.push_back(p);
    }

    // Reset acceleration on all particles; decrease life
    for (auto &p: out_particles->particles) {
      p.ax = p.ay = 0.f;
      // Decrease life by dt
      p.life -= dt;
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
      }
    }

    // Update all particles' positions
    for (auto &p: out_particles->particles) {
      // Update velocity based on acceleration
      p.vx += p.ax * dt;
      p.vy += p.ay * dt;

      // Update position based on velocity
      p.x += p.vx * dt;
      p.y += p.vy * dt;
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

  [[nodiscard]] float get_param(const std::string &/*param_name*/) const override {
    // No parameters for now, so always return 0
    return 0.f;
  }

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph &graph, CommandHistory & /*history*/) override {
    static constexpr const char *kForceTypes[] = {
      "Acceleration X",
      "Acceleration Y",
      "Radial",
      "Drag X",
      "Drag Y",
      "Vortex",
    };

    if (inputs.size() <= 1) {
      ImGui::TextDisabled("No input forces.");
    }

    if (ImGui::Button(ICON_FA_PLUS "  Add Force")) {
      Pin *force_pin = add_input(DataType::Float, "force " + std::to_string(inputs.size()));
      force_pin->id = graph.pin_id_generator.generate_id();
      force_pin->pad = static_cast<uint8_t>(ParticleSystemForce::AccelerationY);
    }

    for (size_t i = 1; i < inputs.size(); ++i) {
      const char *const pin_name = inputs[i].name.c_str();
      ImGui::PushID(inputs[i].id);
      ImGui::TextUnformatted(pin_name);
      ImGui::SameLine();

      ImGui::BeginDisabled(is_renaming);

      // Combo for selecting force type
      int force_type = inputs[i].pad - (uint8_t) ParticleSystemForce::AccelerationY;
      if (ImGui::Combo("Force type", &force_type, kForceTypes, 6)) {
        inputs[i].pad = static_cast<uint8_t>((int) ParticleSystemForce::AccelerationY + force_type);
      }
      ImGui::SameLine();

      // Small rename button
      if (ImGui::SmallButton(ICON_FA_I_CURSOR)) {
        is_renaming = true;
        rename_pin_id = inputs[i].id;
      }

      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Rename this force pin (%s)", pin_name);
      }

      // small red remove button
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.f));
      if (ImGui::SmallButton(ICON_FA_XMARK)) {
        remove_input(i);
      }

      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Remove this force pin (%s)", pin_name);
      }

      ImGui::EndDisabled();

      ImGui::PopStyleColor();
      ImGui::PopID();
    }

    const std::string popup_id = "RenamePinPopup" + std::to_string(id);

    if (is_renaming) {
      ImGui::OpenPopup(popup_id.c_str());
    }

    if (ImGui::BeginPopup(popup_id.c_str())) {
      // Focus the text input when the popup opens
      Pin *force_pin = get_input_by_id(rename_pin_id);
      if (ImGui::IsWindowAppearing()) {
        if (!force_pin) {
          ImGui::TextColored(ImVec4(200, 50, 50, 255), "Pin %d not found!", rename_pin_id);
        } else {
          ImGui::SetKeyboardFocusHere();
          // Upon popping up the rename popup, fill the buffer with the current name
          strncpy(pin_name_buf, force_pin->name.c_str(), sizeof(pin_name_buf));
        }
      }

      if (force_pin) {
        ImGui::InputText("##renamePinNewName", pin_name_buf, sizeof(pin_name_buf));

        ImGui::SameLine();

        ImGui::BeginDisabled(strlen(pin_name_buf) == 0);
        if (ImGui::Button("OK")) {
          const std::string new_name(pin_name_buf);
          force_pin->name = pin_name_buf;
          ImGui::CloseCurrentPopup();
          is_renaming = false;
        }
        ImGui::EndDisabled();
      }

      ImGui::EndPopup();
    }
  }

  /// Factory method to create a new ParticleSystemNode with the correct input and output pins
  /// The input pin will be for the delta particle data, and the output pin will be for the processed particle data to be rendered
  static std::unique_ptr<Node> create() {
    auto node = std::make_unique<ParticleSystemNode>();

    node->add_input(DataType::Particles2D, "new_particles");
    node->add_output(DataType::Particles2D, "out_particles");

    return node;
  }

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
};


#endif //NAG_ENGINE_PARTICLE_SYSTEM_NODE_H
