#ifndef NAG_ENGINE_NODES_NODE_TYPE_NAMES_H
#define NAG_ENGINE_NODES_NODE_TYPE_NAMES_H

#include <array>
#include <string_view>

#include "spdlog/spdlog.h"

#include "engine/nodes/node.h"

/**
 * @brief Compile-time name table for NodeType serialization.
 *
 * This array is indexed directly by the underlying integer value of NodeType
 * (i.e. kNodeTypeNames[static_cast<size_t>(NodeType::Circle)] == "Circle").
 * This gives O(1) lookup in both directions with no risk of duplicate entries,
 * because each NodeType value maps to exactly one slot.
 *
 * INVARIANT: NodeType enum values must be contiguous, start at 0, and have no
 * explicit integer assignments. If reordering or removing existing entries,
 * edit this file accordingly.
 * When adding new NodeType values, please make sure you add them before Count
 * (see node.h).
 * Violating this would silently mismap names to types, corrupting saved graphs.
 *
 * When a new NodeType is added to node.h, add the corresponding name string
 * at the same position in this array. The static_assert below will catch any
 * size mismatch at compile time.
 */
inline constexpr std::array<std::string_view,
  static_cast<size_t>(NodeType::Count)> kNodeTypeNames = {{
  "Default",

  // Generators
  "Constant",
  "Time",
  "Noise",
  "Random",
  "Step Sequencer",

  // Unary math operators
  "Abs",
  "Floor",
  "Ceil",
  "Round",
  "Sqrt",
  "Negate",

  // Trigonometric functions
  "Sin",
  "Cos",
  "Tan",

  // Binary math operators
  "Subtract",
  "Multiply",
  "Divide",
  "Modulo",
  "Power",

  // N-ary math operators
  "Add",
  "Min",
  "Max",

  // Special operators
  "Remap",
  "Clamp",
  "Lerp",
  "Smooth Step",

  // Temporal modifiers
  "LFO",
  "Envelope",
  "Delay",
  "Smoother",

  // Visual nodes
  "Clear Color",
  "Gradient",
  "Circle",
  "Ellipse",
  "Rectangle 2D",
  "Polygon",
  "Texture Loader",
  "Tile",

  // FX nodes
  "Blur",
  "Chromatic Aberration",
  "Color Correction",
  "Composite",
  "Displace",
  "Pixelate",
  "SDFShape",
  "Transform",

  // Sink
  "Output",
}};

static_assert(kNodeTypeNames.size() == static_cast<size_t>(NodeType::Count),
  "kNodeTypeNames size must match NodeType::Count. "
  "Did you add a NodeType without updating kNodeTypeNames?");

/**
 * @brief Convert a NodeType to its stable string name for serialization.
 * @param type A valid NodeType value (must be < NodeType::Count).
 * @return The string name, e.g. "Circle". Returns "Default" for out-of-range values.
 */
[[nodiscard]] inline std::string_view node_type_to_string(const NodeType type) {
  const auto index = static_cast<size_t>(type);
  if (index >= kNodeTypeNames.size()) {
    spdlog::warn("node_type_to_string: unknown NodeType value {}, using Default", index);
    return kNodeTypeNames[0];
  }
  return kNodeTypeNames[index];
}

/**
 * @brief Convert a serialized name back to a NodeType.
 *
 * Linear scan over kNodeTypeNames — acceptable given the small table size.
 *
 * @param name The string name as written by node_type_to_string.
 * @return The matching NodeType, or NodeType::Default with a warning if not found.
 *         Returning Default (rather than throwing) keeps loading safe when opening
 *         graphs saved by a newer version of the engine that has types unknown to
 *         the current build.
 */
[[nodiscard]] inline NodeType node_type_from_string(const std::string_view name) {
  for (size_t i = 0; i < kNodeTypeNames.size(); ++i) {
    if (kNodeTypeNames[i] == name) {
      return static_cast<NodeType>(i);
    }
  }
  spdlog::warn("node_type_from_string: unknown NodeType name '{}', using Default",
               name);
  return NodeType::Default;
}

#endif // NAG_ENGINE_NODES_NODE_TYPE_NAMES_H