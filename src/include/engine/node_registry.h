#ifndef NAG_ENGINE_NODE_REGISTRY_H
#define NAG_ENGINE_NODE_REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
  NodeType    type;
  std::string display_name;
  std::string category;
  std::string description;

  std::function<std::unique_ptr<Node>()> create_func;
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
  [[nodiscard]] std::string_view node_type_to_string(const NodeType type) {
    const auto it = registry_.find(type);
    return it != registry_.end() ? std::string_view(it->second.display_name) : "Default";
  }

  [[nodiscard]] NodeType find_type_by_name(const std::string &name) const {
    for (const auto &[type, info] : registry_) {
      if (info.display_name == name) {
        return type;
      }
    }
    return NodeType::Default; // Return Default if not found
  }

private:
  NodeRegistry() = default;

  std::unordered_map<NodeType, NodeTypeInfo> registry_;
};

void register_all_builtin_nodes();

#endif // NAG_ENGINE_NODE_REGISTRY_H
