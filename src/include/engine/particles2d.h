#ifndef NAG_ENGINE_PARTICLES2D_H
#define NAG_ENGINE_PARTICLES2D_H

#include <vector>

struct Particle2D {
  float x{0.f}, y{0.f};
  float vx{0.f}, vy{0.f};
  float ax{0.f}, ay{0.f};
  float life{0.f};
  float max_life{0.f};
};

struct Particles2D {
  float dt{0.f}; // Time step for the current update, useful for nodes that consume particle data
  std::vector<Particle2D> particles;
};

#endif // NAG_ENGINE_PARTICLES2D_H
