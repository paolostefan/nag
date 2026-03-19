#include "engine/serialization/json_graph_serializer.h"

#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#include "engine/node_registry.h"
#include "engine/nodes/node_type_names.h"

using json = nlohmann::json;

namespace {
  /**
   * @brief Get current timestamp as ISO 8601 string.
   */
  std::string get_iso_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
  }
} // namespace

OperationResult JsonGraphSerializer::save(
  const NodeGraph &graph,
  const std::string &path
) const {
  try {
    json j;

    // Metadata
    j["version"] = kFormatVersion;
    j["metadata"] = {
      {"created", get_iso_timestamp()},
      {"node_count", graph.nodes.size()},
      {"link_count", graph.links.size()}
    };

    // Serialize nodes
    j["nodes"] = json::array();
    for (const auto &node: graph.nodes) {
      j["nodes"].push_back(serialize_node(node.get()));
    }

    // Serialize links
    j["links"] = json::array();
    for (const auto &link: graph.links) {
      // Find start pin index
      const Node *start_node = nullptr;
      size_t start_pin_index = 0;
      bool start_found = false;

      for (const auto &node: graph.nodes) {
        for (size_t i = 0; i < node->outputs.size(); ++i) {
          if (node->outputs[i].id == link.start_pin_id) {
            start_node = node.get();
            start_pin_index = i;
            start_found = true;
            break;
          }
        }
        if (start_found) break;
      }

      // Find end pin index
      const Node *end_node = nullptr;
      size_t end_pin_index = 0;
      bool end_found = false;

      for (const auto &node: graph.nodes) {
        for (size_t i = 0; i < node->inputs.size(); ++i) {
          if (node->inputs[i].id == link.end_pin_id) {
            end_node = node.get();
            end_pin_index = i;
            end_found = true;
            break;
          }
        }
        if (end_found) break;
      }

      if (!start_found || !end_found) {
        spdlog::warn("Link {} has invalid pin references, skipping", link.id);
        continue;
      }

      j["links"].push_back({
        {"id", link.id},
        {"start_node_id", start_node->id},
        {"start_pin_index", start_pin_index},
        {"end_node_id", end_node->id},
        {"end_pin_index", end_pin_index}
      });
    }

    // Write to file
    std::ofstream file(path);
    if (!file.is_open()) {
      return OperationResult::error("Failed to open file for writing: " + path);
    }

    file << j.dump(2); // Pretty print with 2-space indent
    file.close();

    spdlog::info("Saved graph to {}", path);
    return OperationResult::ok();
  } catch (const std::exception &e) {
    return OperationResult::error(
      std::string("Exception during save: ") + e.what()
    );
  }
}

OperationResult JsonGraphSerializer::load(
  NodeGraph &graph,
  const std::string &path
) const {
  try {
    // Read file
    std::ifstream file(path);
    if (!file.is_open()) {
      return OperationResult::error("Failed to open file for reading: " + path);
    }

    json j;
    file >> j;
    file.close();

    // Check version
    if (!j.contains("version") || j["version"] != kFormatVersion) {
      return OperationResult::error(
        "Unsupported or missing format version in file"
      );
    }

    // Clear existing graph
    graph.nodes.clear();
    graph.links.clear();

    // ID remapping: old_id -> new_id
    std::unordered_map<int, int> id_remap;

    // Deserialize nodes
    if (!j.contains("nodes") || !j["nodes"].is_array()) {
      return OperationResult::error("Missing or invalid 'nodes' array");
    }

    for (const auto &node_json: j["nodes"]) {
      auto node = deserialize_node(node_json);
      if (!node) {
        return OperationResult::error(
          "Failed to deserialize node with id " +
          std::to_string(node_json.value("id", 0))
        );
      }

      const auto node_ptr = graph.add_node(std::move(node));
      // Track ID remapping (old -> new)
      id_remap[node_json.value("id", 0)] = node_ptr->id;
    }

    // Deserialize links
    if (!j.contains("links") || !j["links"].is_array()) {
      return OperationResult::error("Missing or invalid 'links' array");
    }

    for (const auto &link_json: j["links"]) {
      // Get remapped node IDs
      int old_start_node_id = link_json.value("start_node_id", 0);
      int old_end_node_id = link_json.value("end_node_id", 0);

      auto start_it = id_remap.find(old_start_node_id);
      auto end_it = id_remap.find(old_end_node_id);

      if (start_it == id_remap.end() || end_it == id_remap.end()) {
        spdlog::warn("Link references non-existent node, skipping");
        continue;
      }

      int new_start_node_id = start_it->second;
      int new_end_node_id = end_it->second;

      // Find pins
      size_t start_pin_index = link_json.value("start_pin_index", 0);
      size_t end_pin_index = link_json.value("end_pin_index", 0);

      Pin *start_pin = find_pin(graph, new_start_node_id, start_pin_index, true);
      Pin *end_pin = find_pin(graph, new_end_node_id, end_pin_index, false);

      if (!start_pin || !end_pin) {
        spdlog::warn("Link references invalid pin indices, skipping");
        continue;
      }

      // Create link
      graph.add_link(start_pin, end_pin);
    }

    spdlog::info("Loaded graph from {} ({} nodes, {} links)",
                 path, graph.nodes.size(), graph.links.size());
    return OperationResult::ok();
  } catch (const std::exception &e) {
    return OperationResult::error(
      std::string("Exception during load: ") + e.what()
    );
  }
}

json JsonGraphSerializer::serialize_node(const Node *node) {
  json j;
  j["id"] = node->id;
  j["type"] = node_type_to_string(node->type);
  j["name"] = node->name;
  j["position"] = {node->position.x, node->position.y};

  j["params"] = node->serialize_params();

  return j;
}

std::unique_ptr<Node> JsonGraphSerializer::deserialize_node(const json &j) {
  // Extract basic fields
  int old_id = j.value("id", 0);
  // Deserialize type by name for stability across enum reorderings.
  // Backward-compat: if the field is still an integer (graph saved before this
  // change), fall back to the old numeric cast so existing files keep working.
  NodeType type = NodeType::Default;
  if (j.contains("type")) {
    if (j["type"].is_string()) {
      type = node_type_from_string(j["type"].get<std::string>());
    } else if (j["type"].is_number_integer()) {
      spdlog::warn("Node {}: 'type' is numeric (old format), casting directly", old_id);
      type = static_cast<NodeType>(j["type"].get<int>());
    }
  }
  const std::string name = j.value("name", "<unnamed>");

  ImVec2 position{0.f, 0.f};
  if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 2) {
    position.x = j["position"][0];
    position.y = j["position"][1];
  }

  // Create node via registry
  auto node = NodeRegistry::instance().create_node(type);
  if (!node) {
    spdlog::error("Failed to create node of type {}", static_cast<int>(type));
    return nullptr;
  }

  // Set basic properties
  node->name = name;
  node->position = position;

  // Deserialize parameters using virtual method
  if (j.contains("params")) {
    if (const auto result = node->deserialize_params(j["params"]); !result) {
      spdlog::warn("Failed to deserialize params for node {}: {}",
                   old_id, result.error_message);
    }
  }

  return node;
}

Pin *JsonGraphSerializer::find_pin(NodeGraph &graph,
                                   const int node_id,
                                   const size_t pin_index,
                                   const bool is_output
) {
  for (const auto &node: graph.nodes) {
    if (node->id == node_id) {
      auto &pins = is_output ? node->outputs : node->inputs;
      if (pin_index < pins.size()) {
        return &pins[pin_index];
      }
      break;
    }
  }
  return nullptr;
}
