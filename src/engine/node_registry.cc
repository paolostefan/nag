#include "engine/node_registry.h"

#include "spdlog/spdlog.h"

#include "engine/nodes/node.h"

NodeRegistry &NodeRegistry::instance() {
  static NodeRegistry instance;
  return instance;
}

std::unique_ptr<Node> NodeRegistry::create_node(const NodeType type) const {
  const auto it = registry_.find(type);
  if (it == registry_.end()) {
    spdlog::error("Node type {} not registered", static_cast<int>(type));
    return nullptr;
  }

  return it->second.create_func();
}

const NodeTypeInfo *NodeRegistry::get_type_info(const NodeType type) const {
  const auto it = registry_.find(type);
  return it != registry_.end() ? &it->second : nullptr;
}

std::unordered_map<std::string, std::vector<NodeType> >
NodeRegistry::get_nodes_by_category() const {
  std::unordered_map<std::string, std::vector<NodeType> > result;

  for (const auto &[type, info]: registry_) {
    result[info.category].push_back(type);
  }

  return result;
}

std::vector<NodeType> NodeRegistry::get_all_types() const {
  std::vector<NodeType> result;
  result.reserve(registry_.size());

  for (const auto &type: registry_ | std::views::keys) {
    result.push_back(type);
  }

  return result;
}

void register_all_builtin_nodes() {
  static bool initialized = false;
  if (initialized) return;

  spdlog::info("Registering built-in nodes...");

  node_registration::register_fx_nodes();
  node_registration::register_generator_nodes();
  node_registration::register_math_nodes();
  node_registration::register_temporal_nodes();
  node_registration::register_visual_nodes();

  initialized = true;

  spdlog::info("Registered {} node types",
               NodeRegistry::instance().get_all_types().size());
}
