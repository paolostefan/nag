# Particle System Redesign — Approach A

## Goal

Make the particle system agnostic and flexible so any force (gravity, drag, turbulence, vortex, attractor, custom field) can be applied without modifying the core emitter.

## Constraints

The existing architecture imposes three non-negotiable constraints:

1. **Stateless dataflow**: Nodes pass data through `Stream<Particles2D>` by value copy. Links/edges define a DAG evaluated via topological sort.
2. **No cycles**: Kahn's algorithm in `NodeGraph::evaluate()` rejects cycles. Feedback loops are impossible.
3. **State must live somewhere**: Particles persist across frames (position, velocity, life). This state cannot live "in the stream" — streams are ephemeral per-evaluation.

Any design must respect all three.

## Chosen: Parameter-Input Architecture (Approach A)

### Core Idea

Keep the particle system as a single stateful node, but make all **force parameters** into **external input pins** of type `DataType::Float`. Any upstream node (Constant, Noise, LFO, Random, custom) can drive any force parameter. The particle system node reads these inputs each frame, applies them internally during its step, then integrates.

### Why This Over Alternatives

| Approach | Problem |
|----------|---------|
| **Force as separate Particles2D filter nodes** | Requires a cycle (feedback) or forces have one-frame delay. The emitter's output would need to loop through filters and back, which topo sort rejects. |
| **Shared-state force nodes** (shared_ptr to particle buffer) | Breaks dataflow model entirely. Undo/redo, serialization, graph traversal all assume owned state. |
| **Callback/strategy injection** | Not serializable. Can't be represented in the node graph or saved to `.nag` files. |

Approach A wins because: it uses only `DataType::Float` (already exists), no cycles needed, fully serializable, and any float-producing node can be connected.

### Architecture

**New: `ParticleEmitterNode` (spawn-only)**

Responsibilities:

- Emit new particles at `rate` per second
- Set initial velocity (radial distribution, `speed` magnitude)
- Set particle `life` from uniform random between `min_life` / `max_life`
- Output ONLY particles spawned this frame

No integration, no killing, no persistent state.

```
Outputs:
  [0] "new_particles" (Particles2D) — fresh spawns each evaluate()
```

**New: `ParticleSystemNode` (stateful accumulator)**

Responsibilities:

- Own persistent particle buffer across frames
- Input: receive new spawns from emitter
- Input: read force parameters from pins
- Each evaluate():
  1. Append new particles to internal buffer
  2. Apply forces using input pin values
  3. Integrate (v += a·dt, x += v·dt, life -= dt)
  4. Remove dead particles
  5. Reset acceleration accumulators
  6. Output buffer to stream

```
Inputs:
  [0] "new_particles" (Particles2D)
  [1] "dt"            (float)     — from TimeNode
  [2] "gravity"       (float)     — from ConstantNode, etc.
  [3] "drag"          (float)     — from ConstantNode, etc.
  [4+] Custom force slots — see "Generic Force Slots" below

Outputs:
  [0] "particles" (Particles2D) — all live particles, post-force, post-integration
```

**New: `Particle2D` field additions**

```cpp
struct Particle2D {
  float x{0.f}, y{0.f};
  float vx{0.f}, vy{0.f};
  float ax{0.f}, ay{0.f};  // NEW: force accumulator
  float life{0.f};
  float max_life{0.f};
};
```

`ax`/`ay` let multiple force sources accumulate contributions before integration. Cleared to zero each frame after integration.

### Generic Force Slots

Rather than hardcoding a fixed set of force inputs, the `ParticleSystemNode` should allow adding/removing named force slots at runtime — similar to how `MultiInputNode` adds lettered inputs. Each slot is:

- A user-defined name (string)
- A float input pin with that name
- An optional "force type" parameter (enum: `Acceleration`, `Velocity`, `Drag`, `Noise`, `Radial`, `Vortex`)

The force type determines how the float value is applied in `evaluate()`:

| Type | Application |
|------|-------------|
| `Acceleration` | `p.ay += value * dt` (uniform vertical force) |
| `Velocity` | `p.vy += value * dt` (direct velocity change) |
| `Drag` | `p.vx *= (1 - value*dt)` (exponential damping) |
| `Noise` | `p.ax += noise(p.x, p.y, time) * value` |
| `Radial` | `p.ax += (cx-p.x)/dist * value; p.ay += (cy-p.y)/dist * value` |
| `Vortex` | `p.ax += (cy-p.y)/dist * value; p.ay += -(cx-p.x)/dist * value` |

This makes the system extensible: new force types require adding an enum entry + application code, without changing the graph topology.

The UI (`draw_properties`) allows:

- Adding a new force slot (name + type)
- Removing existing slots
- Editing the force type per slot

Serialization (`serialize_params`/`deserialize_params`) saves the slot list so graphs are portable.

### `ParticleSystemNode::evaluate()` pseudocode

```cpp
void evaluate() override {
  auto t = time_since_last_evaluate(); // or read dt from pin
  dt_ = t;

  // 1. Read new spawns
  if (auto *new_p = inputs[0].get_particles()) {
    buffer_.particles.insert(
      buffer_.particles.end(),
      new_p->particles.begin(),
      new_p->particles.end());
  }

  // 2. Apply each force slot
  for (auto &slot : force_slots_) {
    auto *val = get_input(slot.pin_name)->get_float();
    if (!val) continue;
    apply_force(slot.type, *val, dt_);
  }

  // 3. Integrate
  for (auto &p : buffer_.particles) {
    p.vx += p.ax * dt_;
    p.vy += p.ay * dt_;
    p.x  += p.vx * dt_;
    p.y  += p.vy * dt_;
    p.life -= dt_;
  }

  // 4. Kill dead
  std::erase_if(buffer_.particles,
    [](auto &p) { return p.life <= 0.f; });

  // 5. Reset acceleration
  for (auto &p : buffer_.particles) {
    p.ax = 0.f;
    p.ay = 0.f;
  }

  // 6. Output
  outputs[0].set_particles(buffer_);
  mark_inputs_consumed();
}
```

### Example Graphs

**Fire fountain (gravity + drag):**

```
[TimeNode]──dt──────────────────────────────┐
[Constant: 9.8]──gravity────────────────────┤
[Constant: 0.02]──drag──────────────────────┤
                                            ▼
[ParticleEmitter (rate=100, speed=200)]───►[ParticleSystem]──►[ParticleRenderer]
```

**Swarm with turbulence + attractor:**

```
[TimeNode]──dt────────────────────────────────────┐
[Noise(freq=0.1)]──turbulence─────────────────────┤
[LFO(sin, amp=50)]──attractor_strength────────────┤
[Constant(cx)]────────────attractor_x──────────────┤
[Constant(cy)]────────────attractor_y──────────────┤
                                                   ▼
[ParticleEmitter (rate=20, speed=50)]────────────►[ParticleSystem]──►[ParticleRenderer]
```

**Wind burst (random impulse):**

```
[TimeNode]──dt──────────────────────────────┐
[Random(seed=3, range=-500..500)]──wind_x───┤
                                           ▼
[ParticleEmitter]─────────────────────────►[ParticleSystem]──►[Renderer]
```

### `ParticleEmitterNode` Refactoring

The current `step(float dt)` splits into two:

1. `ParticleEmitterNode::evaluate()` — only spawning, no integration/kill
2. `ParticleSystemNode::evaluate()` — receives spawns, applies forces, integrates, kills

The emitter loses:

- `step()` (entirely — the topo sort calls `evaluate()`)
- Internal `particles_` buffer
- Integration and death logic

It keeps:

- `rate`, `speed`, `min_life`/`max_life`, `max_particles`, `seed`, `rng`, `spawn_accumulator`
- `spawn_particle()` method

The emitter's output pin stream is **ephemeral** — it only contains particles for the current frame, and the accumulator appends them to its persistent buffer on the same evaluation pass.

### `ParticleRendererNode` Changes

Renderer reads `Particles2D` from its input pin as before. No changes needed functionally, but note: the renderer should handle up to `max_particles` particles efficiently (vertex buffer streaming, etc.).

### NodeType Enum & Registry

Two new `NodeType` values added (append before `Count`):

```cpp
ParticleSystem,      // after ParticleEmitter
```

New entry in `kNodeTypeNames` + `register_generator_nodes()` call.

### Serialization

`ParticleSystemNode` serializes:

```json
{
  "max_particles": 10000,
  "force_slots": [
    {"name": "gravity", "type": "acceleration"},
    {"name": "drag", "type": "drag"}
  ]
}
```

Each force slot corresponds to an input pin position. On deserialization, pins are created in order.

### Migration Path

1. Add `ax`, `ay` to `Particle2D`
2. Create `ParticleEmitterNode` (spawn-only) — copy existing, strip state/integration
3. Create `ParticleSystemNode` — implement persistent buffer + force slots + integration
4. Register both in NodeRegistry with new `NodeType` values
5. Keep old `ParticleEmitterNode` temporarily for backward compat (or remove if unused in saved files)
6. Update `ParticleRendererNode` if needed
7. Update test graphs/scenes

### Performance Notes

- Each force slot reads one float from an input pin (negligible cost)
- ParticleSystem does one pass over the buffer per force slot. With 10 force slots and 10k particles: 100k iterations — well within budget at 60fps
- Buffer copy to output stream via `set_particles()` copies the vector once per frame (same as current)
- `ax/ay` reset avoids stale accumulation
