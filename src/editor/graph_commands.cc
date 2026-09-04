#include "editor/graph_commands.h"

#include "spdlog/spdlog.h"

#include "engine/node_registry.h"
#include "engine/serialization/json_graph_serializer.h"

// ============================================================================
// AddNodeCommand
// ============================================================================

AddNodeCommand::AddNodeCommand(std::unique_ptr<Node> node)
  : node_(std::move(node)) {
}

bool AddNodeCommand::execute(NodeGraph &graph) {
  if (!node_) {
    // Redo case: deserialize from stored JSON
    auto recreated = NodeRegistry::instance().create_node(
      static_cast<NodeType>(node_data_["type"].get<int>())
    );

    if (!recreated) {
      spdlog::error("Failed to recreate node for redo");
      return false;
    }

    recreated->name = node_data_["name"];
    recreated->gui_x = node_data_["gui_xy"][0];
    recreated->gui_y = node_data_["gui_xy"][1];

    if (node_data_.contains("params")) {
      recreated->deserialize_params(node_data_["params"]);
    }

    added_node_id_ = recreated->id;
    graph.add_node(std::move(recreated));
  } else {
    // First execute: add the node
    added_node_id_ = node_->id;

    // Serialize for potential redo
    node_data_["type"] = node_->type;
    node_data_["name"] = node_->name;
    node_data_["gui_xy"] = {node_->gui_x, node_->gui_y};
    node_data_["params"] = node_->serialize_params();

    graph.add_node(std::move(node_));
  }

  return true;
}

bool AddNodeCommand::undo(NodeGraph &graph) {
  // Find and remove the node
  const auto it = std::ranges::find_if(graph.nodes,
                                       [this](const std::unique_ptr<Node> &n) {
                                         return n->id == added_node_id_;
                                       });

  if (it == graph.nodes.end()) {
    spdlog::error("Cannot undo AddNode: node {} not found", added_node_id_);
    return false;
  }

  // Remove associated links
  std::erase_if(graph.links, [this, &graph](const Link &link) {
    // Check if link connects to this node
    for (const auto &node: graph.nodes) {
      if (node->id != added_node_id_) continue;

      for (const auto &pin: node->inputs) {
        if (pin.id == link.end_pin_id) return true;
      }
      for (const auto &pin: node->outputs) {
        if (pin.id == link.start_pin_id) return true;
      }
    }
    return false;
  });

  graph.nodes.erase(it);
  return true;
}

std::string AddNodeCommand::description() const {
  return "Add " + node_data_.value("name", "node");
}

// ============================================================================
// DeleteNodesCommand
// ============================================================================

DeleteNodesCommand::DeleteNodesCommand(std::unordered_set<int> node_ids)
  : node_ids_(std::move(node_ids)) {
}

bool DeleteNodesCommand::execute(NodeGraph &graph) {
  if (deleted_nodes_.empty()) {
    // First execute: capture state for undo
    JsonGraphSerializer serializer;

    for (const auto &node: graph.nodes) {
      if (!node_ids_.contains(node->id)) continue;

      // Serialize node
      nlohmann::json node_json;
      node_json["id"] = node->id;
      node_json["type"] = node->type;
      node_json["name"] = node->name;
      node_json["gui_xy"] = {node->gui_x, node->gui_y};
      node_json["params"] = node->serialize_params();
      deleted_nodes_.push_back(node_json);

      // Capture connected links with pin indices for reliable undo
      for (const auto &link: graph.links) {
        int src_node_id = 0, dst_node_id = 0;
        int src_pin_index = -1, dst_pin_index = -1;

        for (const auto &node_: graph.nodes) {
          for (int i = 0; i < static_cast<int>(node_->outputs.size()); ++i) {
            if (node_->outputs[i].id == link.start_pin_id) {
              src_node_id = node_->id;
              src_pin_index = i;
            }
          }
          for (int i = 0; i < static_cast<int>(node_->inputs.size()); ++i) {
            if (node_->inputs[i].id == link.end_pin_id) {
              dst_node_id = node_->id;
              dst_pin_index = i;
            }
          }
        }

        // Only capture links where at least one endpoint is a node being deleted
        const bool src_deleted = node_ids_.contains(src_node_id);
        const bool dst_deleted = node_ids_.contains(dst_node_id);
        if ((src_deleted || dst_deleted) && src_node_id != 0 && dst_node_id != 0) {
          deleted_links_by_index_.push_back({
            src_node_id, src_pin_index,
            dst_node_id, dst_pin_index
          });
          deleted_links_.push_back(link);
        }
      }
    }
  }

  // Delete links
  std::erase_if(graph.links, [this](const Link &link) {
    return std::ranges::any_of(deleted_links_,
                               [&link](const Link &dl) { return dl.id == link.id; });
  });

  // Delete nodes
  std::erase_if(graph.nodes, [this](const std::unique_ptr<Node> &node) {
    return node_ids_.contains(node->id);
  });

  return true;
}

bool DeleteNodesCommand::undo(NodeGraph &graph) {
  // Recreate nodes
  std::unordered_map<int, int> id_remap;

  for (const auto &node_json: deleted_nodes_) {
    auto node = NodeRegistry::instance().create_node(
      static_cast<NodeType>(node_json["type"].get<int>())
    );

    if (!node) continue;

    const int old_id = node_json["id"];

    node->name = node_json["name"];
    node->gui_x = node_json["gui_xy"][0];
    node->gui_y = node_json["gui_xy"][1];

    if (node_json.contains("params")) {
      node->deserialize_params(node_json["params"]);
    }

    Node *added = graph.add_node(std::move(node));
    id_remap[old_id] = added->id;
  }

  // Recreate links using pin indices and remapped node IDs
  for (const auto &sl: deleted_links_by_index_) {
    // Resolve new node IDs (nodes not deleted keep their original ID)
    const int new_src_id = id_remap.contains(sl.src_node_id)
                             ? id_remap.at(sl.src_node_id)
                             : sl.src_node_id;
    const int new_dst_id = id_remap.contains(sl.dst_node_id)
                             ? id_remap.at(sl.dst_node_id)
                             : sl.dst_node_id;

    const Pin *start_pin = nullptr;
    const Pin *end_pin = nullptr;

    for (const auto &node: graph.nodes) {
      if (node->id == new_src_id &&
          sl.src_pin_index < static_cast<int>(node->outputs.size())) {
        start_pin = &node->outputs[sl.src_pin_index];
      }
      if (node->id == new_dst_id &&
          sl.dst_pin_index < static_cast<int>(node->inputs.size())) {
        end_pin = &node->inputs[sl.dst_pin_index];
      }
    }

    if (start_pin && end_pin) {
      graph.add_link(*start_pin, *end_pin);
    } else {
      spdlog::warn("DeleteNodesCommand::undo: could not restore link "
                   "(node {} pin {} → node {} pin {})",
                   new_src_id, sl.src_pin_index,
                   new_dst_id, sl.dst_pin_index);
    }
  }
  return true;
}

std::string DeleteNodesCommand::description() const {
  return "Delete " + std::to_string(node_ids_.size()) + " node(s)";
}

// ============================================================================
// AddLinkCommand
// ============================================================================

AddLinkCommand::AddLinkCommand(const int start_pin_id, const int end_pin_id)
  : start_pin_id_(start_pin_id), end_pin_id_(end_pin_id) {
}

bool AddLinkCommand::execute(NodeGraph &graph) {
  const Pin *start_pin = nullptr;
  const Pin *end_pin = nullptr;

  // Find pins
  for (const auto &node: graph.nodes) {
    for (auto &pin: node->outputs) {
      if (pin.id == start_pin_id_) start_pin = &pin;
    }
    for (auto &pin: node->inputs) {
      if (pin.id == end_pin_id_) end_pin = &pin;
    }
  }

  if (!start_pin || !end_pin) {
    spdlog::error("Cannot create link: pins not found");
    return false;
  }

  if (!graph.add_link(start_pin, end_pin)) return false;

  // Capture link ID
  for (const auto &link: graph.links) {
    if (link.start_pin_id == start_pin_id_ &&
        link.end_pin_id == end_pin_id_) {
      created_link_id_ = link.id;
      break;
    }
  }

  return true;
}

bool AddLinkCommand::undo(NodeGraph &graph) {
  // Find and remove the link
  const auto it = std::ranges::find_if(graph.links,
                                       [this](const Link &link) { return link.id == created_link_id_; });

  if (it == graph.links.end()) {
    return false;
  }

  // Detach stream from input pin
  for (const auto &node: graph.nodes) {
    for (auto &pin: node->inputs) {
      if (pin.id == it->end_pin_id) {
        pin.stream = nullptr;
        break;
      }
    }
  }

  graph.links.erase(it);
  return true;
}

std::string AddLinkCommand::description() const {
  return "Add link";
}

// ============================================================================
// DeleteLinksCommand
// ============================================================================

DeleteLinksCommand::DeleteLinksCommand(std::unordered_set<int> link_ids)
  : link_ids_(std::move(link_ids)) {
}

DeleteLinksCommand::DeleteLinksCommand(const int link_id)
  : link_ids_(std::unordered_set{link_id}) {
}

bool DeleteLinksCommand::execute(NodeGraph &graph) {
  if (deleted_links_.empty()) {
    // Capture links for undo
    for (const auto &link: graph.links) {
      if (link_ids_.contains(link.id)) {
        deleted_links_.push_back(link);
      }
    }
  }

  // Remove links
  for (const auto &link: deleted_links_) {
    graph.remove_link(link.id);
  }

  std::erase_if(graph.links, [this](const Link &link) {
    return link_ids_.contains(link.id);
  });

  return true;
}

bool DeleteLinksCommand::undo(NodeGraph &graph) {
  // Recreate links
  for (const auto &link: deleted_links_) {
    const Pin *start_pin = nullptr;
    const Pin *end_pin = nullptr;

    for (const auto &node: graph.nodes) {
      for (auto &pin: node->outputs) {
        if (pin.id == link.start_pin_id) start_pin = &pin;
      }
      for (auto &pin: node->inputs) {
        if (pin.id == link.end_pin_id) end_pin = &pin;
      }
    }

    if (start_pin && end_pin) {
      graph.add_link(start_pin, end_pin);
    }
  }

  return true;
}

std::string DeleteLinksCommand::description() const {
  return "Delete " + std::to_string(link_ids_.size()) + " link(s)";
}

// ============================================================================
// MoveNodesCommand
// ============================================================================

MoveNodesCommand::MoveNodesCommand(std::vector<NodePosition> moves)
  : moves_(std::move(moves)) {
}

bool MoveNodesCommand::execute(NodeGraph &graph) {
  for (const auto &move: moves_) {
    for (const auto &node: graph.nodes) {
      if (node->id == move.node_id) {
        node->gui_x = move.new_pos.x;
        node->gui_y = move.new_pos.y;
        break;
      }
    }
  }
  return true;
}

bool MoveNodesCommand::undo(NodeGraph &graph) {
  for (const auto &move: moves_) {
    for (const auto &node: graph.nodes) {
      if (node->id == move.node_id) {
        node->gui_x = move.old_pos.x;
        node->gui_y = move.old_pos.y;
        break;
      }
    }
  }
  return true;
}

std::string MoveNodesCommand::description() const {
  return "Move " + std::to_string(moves_.size()) + " node(s)";
}
