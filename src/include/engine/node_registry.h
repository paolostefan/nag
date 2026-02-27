#ifndef NAG_ENGINE_NODE_REGISTRY_H
#define NAG_ENGINE_NODE_REGISTRY_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "spdlog/spdlog.h"

// Forward declarations
struct Node;
enum class NodeType : uint8_t;

namespace node_registration {

  void register_effect_nodes();

  void register_generator_nodes();

  void register_math_nodes();

  void register_temporal_nodes();

  void register_visual_nodes();
}


/**
 * @brief Metadata for a node type.
 */
struct NodeTypeInfo {
  NodeType type;
  std::string display_name;
  std::string category; // "Generators", "Math", "Temporal", "Visual", etc.
  std::string description;

  // Function pointer to create node
  std::unique_ptr<Node> (*create_func)();
};

/**
 * @brief Factory and metadata registry for all node types.
 *
 * This singleton manages:
 * - Node creation from NodeType enum
 * - Node metadata (name, category, description)
 *
 * Serialization is handled via virtual functions in Node base class.
 */
class NodeRegistry {
public:
  /**
   * @brief Get singleton instance.
   */
  static NodeRegistry &instance();

  /**
 * @brief Register a node type using its static create() method.
 *
 * @tparam NodeClass The node class (must have static create() method)
 * @param type Node type enum value
 * @param display_name Human-readable name
 * @param category Category for UI grouping
 * @param description Short description
 */
  template<typename NodeClass>
  void register_node(
    NodeType type,
    const std::string &display_name,
    const std::string &category,
    const std::string &description) {
    // compile-time check that NodeClass is derived from Node
    static_assert(
      std::is_base_of_v<Node, NodeClass>,
      "NodeClass must derive from Node"
    );

    if (registry_.contains(type)) {
      spdlog::warn("Node type {} already registered, overwriting",
                   static_cast<int>(type));
    }

    // Create function pointer type-erased
    auto create_fn = +[]() -> std::unique_ptr<Node> {
      return NodeClass::create();
    };

    registry_[type] = {
      type,
      display_name,
      category,
      description,
      create_fn
    };

    spdlog::debug("Registered node type: {} ({})",
                  display_name,
                  static_cast<int>(type));
  }

  /**
   * @brief Create a node instance from its type.
   *
   * @param type Node type enum value
   * @return Unique pointer to created node, or nullptr if type not registered
   */
  [[nodiscard]] std::unique_ptr<Node> create_node(NodeType type) const;

  /**
   * @brief Get metadata for a node type.
   *
   * @param type Node type enum value
   * @return Pointer to metadata, or nullptr if not registered
   */
  [[nodiscard]] const NodeTypeInfo *get_type_info(NodeType type) const;

  /**
   * @brief Get all registered node types grouped by category.
   *
   * @return Map of category -> list of node types
   */
  [[nodiscard]] std::unordered_map<std::string, std::vector<NodeType> >
  get_nodes_by_category() const;

  /**
   * @brief Get all registered node types as a flat list.
   *
   * @return Vector of all registered node types
   */
  [[nodiscard]] std::vector<NodeType> get_all_types() const;

private:
  NodeRegistry() = default;

  std::unordered_map<NodeType, NodeTypeInfo> registry_;
};

/**
 * @brief Register all built-in node types.
 *
 * This function must be called once at startup (e.g., in main() or test setup)
 * to ensure all node types are registered.
 */
void register_all_builtin_nodes();

#endif  // NAG_ENGINE_NODE_REGISTRY_H
