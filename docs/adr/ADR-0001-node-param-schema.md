# ADR-0001: Declarative node param schema drives all three consumers

## Status

Accepted

## Date

2026-09-07

## Context

Before this change, a node's scalar parameters were declared in four places:

1. The struct fields in the node header (e.g. `TileNode::tile_x`).
2. `get_param()` — a hand-written `if (name == ...) return ...` chain, used by
   `ShaderNode::bind_float_inputs()` for default values of unconnected float pins.
3. `serialize_params()` / `deserialize_params()` — per-node `try/catch` JSON code.
4. `node_properties_renderers.cc` — a 55-arm `switch (node.type)` with ~110
   `dynamic_cast`s, one row per property, plus 8 raw `ImGui::` edits that
   bypassed undo.

Adding a parameter meant editing two or more of these. The renderers switch in
particular grew per node type — leverage fell, locality was split between the
engine header and the editor switch.

## Decision

Give `Node` a single declarative parameter table, `properties()`, returning
`std::vector<Property`. Each `Property` row carries:

- `name` (JSON key, `get_param` lookup key, undo param name)
- `label` (panel display name)
- `kind` (slider, drag, input, combo, color, checkbox — an engine-side enum)
- min/max/format, optional combo items, optional `disable_pin`
- engine-side flags (e.g. `Logarithmic`)
- getter/setter `std::function`s bound to the node

Three consumers read that one table:

1. The editor property adapter (`switch (kind)` → `PropertyWidget`, which
   preserves the undo path via `SetNodeParamCommand`).
2. `serialize_params()` / `deserialize_params()` reimplemented in the `Node`
   base as a schema loop.
3. `get_param()` as a schema lookup, returning float-coerced values.

The schema lives in engine headers as pure data — no ImGui types, `Vec4` for
colors, an engine-side flag enum. The ImGui conversion is done by the editor
adapter. `visual_types.h` drops its `imgui.h` include.

## Constraints

- **JSON format must be preserved.** The schema loop emits the same keys and
  value shapes as the per-node code it replaces, so existing `.nag` files keep
  loading and `node_serialization_test.cc` remains a valid pin.
- Nodes that do genuinely structural UI keep a per-node editor hook:
  `TextureLoader` (file dialog), `ParticleSystem` (dynamic pins), `SDFShape`
  (shape-conditional sections). Everything else rides the schema.
- Composite values (e.g. `MandelNode::center`, serialized as one `[x,y]` array
  but rendered as two sliders) keep a per-node pack/unpack hook.

## Consequences

Positive:

- One declaration per parameter; three consumers read it.
- New node types no longer require an editor switch arm.
- The 8 raw-ImGui edits become schema rows → undo coverage for all properties.
- Engine is free of ImGui types (completes the earlier decoupling work).
- The schema is testable headlessly through pure accessors.

Negative:

- `Property` table boilerplate per node (constructor-built vector).
- Composite pack hooks remain hand-written per node.
- `get_param` gains float coercion semantics (int → float) for non-optional
  callers; shader binding only ever queries float pins so this is safe.

## Alternatives considered

- Schema drives widgets only: left serialization and `get_param` as parallel
  hand-maintained copies, so the duplication survived.
- Type-erased undo command: rewrote existing typed `PropertyWidget`/`SetNodeParamCommand`
  machinery for no new leverage.
- Schema as `static constexpr` table: cannot hold per-instance closures.