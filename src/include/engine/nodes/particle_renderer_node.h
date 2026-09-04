#ifndef NAG_ENGINE_PARTICLE_RENDERER_NODE_H
#define NAG_ENGINE_PARTICLE_RENDERER_NODE_H

#include <algorithm>
#include <memory>

#include "GL/glew.h"

#include "engine/nodes/visual_node.h"
#include "engine/particles2d.h"
#include "engine/shader_manager.h"
#include "shaders/particle_renderer_frag.h"
#include "shaders/particle_renderer_vert.h"
#include "shaders/particle_sprite_frag.h"

struct ParticleRendererNode : VisualNode {
  float color_jitter{0.f};
  float alpha_jitter{0.f};
  float size_min{2.f};
  float size_max{6.f};
  float size_scatter{0.f};
  float global_scale{1.f};
  float emitter_x{0.5f};
  float emitter_y{0.5f};

  GLuint vao_{0};
  GLuint vbo_{0};
  std::shared_ptr<ShaderProgram> particle_shader_;
  std::shared_ptr<ShaderProgram> sprite_shader_;

  struct Vertex {
    float x, y;
    float r, g, b, a;
    float size;
  };

  ParticleRendererNode() {
    type = NodeType::ParticleRenderer;
    name = "Particle Renderer";
  }

  ~ParticleRendererNode() override {
    if (vao_)
      glDeleteVertexArrays(1, &vao_);
    if (vbo_)
      glDeleteBuffers(1, &vbo_);
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Particle Renderer"; }

  bool initialize(const int width, const int height) override {
    if (!VisualNode::initialize(width, height)) return false;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void *>(offsetof(Vertex, r)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void *>(offsetof(Vertex, size)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    particle_shader_ = ShaderManager::instance().load_from_source(
      "particle_renderer", kparticle_renderer_vert, kparticle_renderer_frag
    );

    sprite_shader_ = ShaderManager::instance().load_from_source(
      "particle_sprite", kparticle_renderer_vert, kparticle_sprite_frag
    );

    return particle_shader_ && particle_shader_->is_valid() &&
           sprite_shader_ && sprite_shader_->is_valid();
  }

  void render() override {
    if (!render_target || !render_target->is_valid()) return;

    render_target->clear(0.f, 0.f, 0.f, 0.f);

    Pin *sprite_pin = get_input("sprite");
    Texture *const *sprite_tex = sprite_pin ? sprite_pin->get_texture() : nullptr;
    render_particles(sprite_tex && *sprite_tex ? *sprite_tex : nullptr);

    // Set alpha to 1.0 do ImGui preview displays FBO as opaque
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_TRUE);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

    RenderTarget::unbind();
  }

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j = VisualNode::serialize_params();
    j["color_jitter"] = color_jitter;
    j["alpha_jitter"] = alpha_jitter;
    j["size_min"] = size_min;
    j["size_max"] = size_max;
    j["size_scatter"] = size_scatter;
    j["global_scale"] = global_scale;
    j["emitter_x"] = emitter_x;
    j["emitter_y"] = emitter_y;
    return j;
  }

  OperationResult deserialize_params(const nlohmann::json &j) override {
    if (const auto result = VisualNode::deserialize_params(j); !result) return result;
    if (j.contains("color_jitter")) color_jitter = j["color_jitter"];
    if (j.contains("alpha_jitter")) alpha_jitter = j["alpha_jitter"];
    if (j.contains("size_min")) size_min = j["size_min"];
    if (j.contains("size_max")) size_max = j["size_max"];
    if (j.contains("size_scatter")) size_scatter = j["size_scatter"];
    if (j.contains("global_scale")) global_scale = j["global_scale"];
    if (j.contains("emitter_x")) emitter_x = j["emitter_x"];
    if (j.contains("emitter_y")) emitter_y = j["emitter_y"];
    return OperationResult::ok();
  }

  static std::unique_ptr<Node> create() {
    auto node = std::make_unique<ParticleRendererNode>();
    node->add_input(DataType::Particles2D, "particles");
    node->add_input(DataType::Texture, "sprite");
    node->add_output(DataType::Texture, "render");
    return node;
  }

private:
  static constexpr unsigned int hash_int(const int i) {
    unsigned h = static_cast<unsigned>(i) * 0x9e3779b9u;
    h = (h ^ (h >> 16)) * 0x85ebca6bu;
    h ^= h >> 13;
    return h;
  }

  static constexpr float hash_float(const int i) {
    return static_cast<float>(hash_int(i)) / 4294967296.0f;
  }

  void render_particles(const Texture *sprite_texture = nullptr) const {
    if (inputs.empty()) return;
    const Particles2D *ps = inputs[0].get_particles();
    if (!ps || ps->particles.empty()) return;

    const auto &particles = ps->particles;
    const size_t count = particles.size();

    if (vbo_ == 0) return;

    // Build vertex data on CPU
    auto vertices = std::make_unique<Vertex[]>(count);

    const float ox = emitter_x * render_target->get_fwidth();
    const float oy = emitter_y * render_target->get_fheight();

    for (size_t i = 0; i < count; ++i) {
      const auto &p = particles[i];
      const float life_t = p.max_life > 0.f ? p.life / p.max_life : 0.f;

      Vertex &v = vertices[i];
      v.x = p.x + ox;
      v.y = p.y + oy;

      const float rj = (hash_float(static_cast<int>(i * 5 + 0)) - 0.5f) * 2.f * color_jitter;
      const float gj = (hash_float(static_cast<int>(i * 5 + 1)) - 0.5f) * 2.f * color_jitter;
      const float bj = (hash_float(static_cast<int>(i * 5 + 2)) - 0.5f) * 2.f * color_jitter;
      v.r = std::clamp(1.f + rj, 0.f, 1.f);
      v.g = std::clamp(1.f + gj, 0.f, 1.f);
      v.b = std::clamp(1.f + bj, 0.f, 1.f);

      const float aj = (hash_float(static_cast<int>(i * 5 + 3)) - 0.5f) * 2.f * alpha_jitter;
      v.a = std::clamp(life_t + aj, 0.f, 1.f);

      const float base_size = size_min + (size_max - size_min) * life_t;
      const float sj = (hash_float(static_cast<int>(i * 5 + 4)) - 0.5f) * 2.f * size_scatter;
      v.size = std::max(1.f, base_size + sj);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(Vertex)),
                 vertices.get(), GL_STREAM_DRAW);

    if (sprite_texture) {
      sprite_shader_->use();
      sprite_shader_->set_uniform("uResolution",
                                  render_target->get_fwidth(),
                                  render_target->get_fheight());
      sprite_shader_->set_uniform("uGlobalScale", global_scale);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sprite_texture->texture_id);
      sprite_shader_->set_uniform("uSpriteTexture", 0);
    } else {
      particle_shader_->use();
      particle_shader_->set_uniform("uResolution",
                                    render_target->get_fwidth(),
                                    render_target->get_fheight());
      particle_shader_->set_uniform("uGlobalScale", global_scale);
    }

    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(count));

    if (sprite_texture)
      glBindTexture(GL_TEXTURE_2D, 0);
    ShaderProgram::unuse();
    glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
  }
};

#endif // NAG_ENGINE_PARTICLE_RENDERER_NODE_H
