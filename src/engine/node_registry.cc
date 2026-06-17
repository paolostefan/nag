#include "engine/node_registry.h"

#include "spdlog/spdlog.h"

#include "engine/nodes/node.h"

NodeRegistry &NodeRegistry::instance() {
  static NodeRegistry instance;
  return instance;
}

void NodeRegistry::register_node(
  const NodeType type,
  const std::string &category,
  const std::string &description,
  const std::function<std::unique_ptr<Node>()> create_func
) {
  const auto idx = static_cast<size_t>(type);
  if (registry_[idx].registered) {
    spdlog::warn("Node type {} already registered, overwriting",
                 static_cast<int>(type));
  }

  const auto temp = create_func();
  if (!temp) {
    spdlog::error("Failed to create node instance for type {} during registration",
                  static_cast<int>(type));
    return;
  }

  registry_[idx] = NodeTypeInfo{
    std::string(temp->type_name()),
    category,
    description,
    create_func,
    true
  };

  spdlog::debug("Registered node type: {} ({})",
                temp->type_name(),
                static_cast<int>(type));
}

std::unique_ptr<Node> NodeRegistry::create_node(const NodeType type) const {
  const auto &info = registry_[static_cast<size_t>(type)];
  if (!info.registered) {
    spdlog::error("Node type {} not registered", static_cast<int>(type));
    return nullptr;
  }

  return info.create_func();
}

const NodeTypeInfo *NodeRegistry::get_type_info(const NodeType type) const {
  const auto &info = registry_[static_cast<size_t>(type)];
  return info.registered ? &info : nullptr;
}

std::unordered_map<std::string, std::vector<NodeType> >
NodeRegistry::get_nodes_by_category() const {

  static std::unordered_map<std::string, std::vector<NodeType> > result;

  if (!result.empty()) {
    return result;
  }

  for (size_t i = 0; i < kNumNodeTypes; ++i) {
    if (!registry_[i].registered) continue;
    result[registry_[i].category].push_back(static_cast<NodeType>(i));
  }

  return result;
}

std::vector<NodeType> NodeRegistry::get_all_types() const {
  std::vector<NodeType> result;
  for (size_t i = 0; i < kNumNodeTypes; ++i) {
    if (registry_[i].registered)
      result.push_back(static_cast<NodeType>(i));
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
