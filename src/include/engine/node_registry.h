#ifndef NAG_ENGINE_NODE_REGISTRY_H
#define NAG_ENGINE_NODE_REGISTRY_H

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"

#include "engine/node_type.h"

struct Node;

namespace node_registration {
  void register_fx_nodes();

  void register_generator_nodes();

  void register_math_nodes();

  void register_temporal_nodes();

  void register_visual_nodes();
} // namespace node_registration

struct NodeTypeInfo {
  std::string display_name;
  std::string category;
  std::string description;

  std::function<std::unique_ptr<Node>()> create_func;
  bool registered = false;
};

class NodeRegistry {
public:
  static NodeRegistry &instance();

  void register_node(
    NodeType                               type,
    const std::string &                    category,
    const std::string &                    description,
    std::function<std::unique_ptr<Node>()> create_func
  );

  [[nodiscard]] std::unique_ptr<Node> create_node(NodeType type) const;

  [[nodiscard]] const NodeTypeInfo *get_type_info(NodeType type) const;

  [[nodiscard]] std::unordered_map<std::string, std::vector<NodeType> >
  get_nodes_by_category() const;

  [[nodiscard]] std::vector<NodeType> get_all_types() const;

  /**
   * @brief Convert a NodeType to its stable string name for serialization.
   * @param type A valid NodeType value (must be < NodeType::Count).
   * @return The string name, e.g. "Circle". Returns "Default" for out-of-range values.
   */
  [[nodiscard]] std::string_view node_type_to_string(const NodeType type) const {
    const auto &info = registry_[static_cast<size_t>(type)];
    return info.registered ? std::string_view(info.display_name) : "Default";
  }

  [[nodiscard]] NodeType find_type_by_name(const std::string &name) const {
    for (size_t i = 0; i < kNumNodeTypes; ++i) {
      if (registry_[i].registered && registry_[i].display_name == name) {
        return static_cast<NodeType>(i);
      }
    }
    return NodeType::Default;
  }

  /**
   * @brief Canonical single-adapter serialization of a node to JSON.
   *
   * The "type" field is written as the node's stable string name (see
   * Node::type_name), never the enum integer, so serialized nodes survive
   * NodeType reordering. JsonGraphSerializer and the undo/redo commands
   * delegate here so the on-disk format lives in exactly one place.
   */
  [[nodiscard]] static nlohmann::json serialize_node(const Node &node);

  /**
   * @brief Deserialize a node from JSON.
   *
   * Accepts the canonical string "type" and, for backward compatibility,
   * the legacy numeric form written by old saves. Returns nullptr when the
   * type is unknown or the node could not be constructed.
   */
  [[nodiscard]] std::unique_ptr<Node> deserialize_node(const nlohmann::json &j) const;

private:
  NodeRegistry() = default;

  static constexpr size_t kNumNodeTypes = static_cast<size_t>(NodeType::Count);
  std::array<NodeTypeInfo, kNumNodeTypes> registry_;
};

void register_all_builtin_nodes();

#endif // NAG_ENGINE_NODE_REGISTRY_H
