#ifndef NAG_ENGINE_NODE_REGISTRY_H
#define NAG_ENGINE_NODE_REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "spdlog/spdlog.h"

#include "engine/nodes/node_type_names.h"

struct Node;
enum class NodeType : uint8_t;

namespace node_registration {

void register_fx_nodes();

void register_generator_nodes();

void register_math_nodes();

void register_temporal_nodes();

void register_visual_nodes();
} // namespace node_registration

struct NodeTypeInfo {
  NodeType type;
  std::string display_name;
  std::string category;
  std::string description;

  std::function<std::unique_ptr<Node>()> create_func;
};

class NodeRegistry {
public:
  static NodeRegistry &instance();

  void register_node(
    NodeType type,
    const std::string &category,
    const std::string &description,
    std::function<std::unique_ptr<Node>()> create_func
  );

  [[nodiscard]] std::unique_ptr<Node> create_node(NodeType type) const;

  [[nodiscard]] const NodeTypeInfo *get_type_info(NodeType type) const;

  [[nodiscard]] std::unordered_map<std::string, std::vector<NodeType>>
  get_nodes_by_category() const;

  [[nodiscard]] std::vector<NodeType> get_all_types() const;

private:
  NodeRegistry() = default;

  std::unordered_map<NodeType, NodeTypeInfo> registry_;
};

void register_all_builtin_nodes();

#endif
