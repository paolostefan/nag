#ifndef NAG_EDITOR_GRAPH_COMMANDS_H
#define NAG_EDITOR_GRAPH_COMMANDS_H

#include <memory>
#include <unordered_set>
#include <vector>

#include "nlohmann/json.hpp"

#include "editor/command.h"
#include "engine/nodes/node.h"
#include "engine/node_graph.h"

struct SerializedLink {
  int src_node_id; // old node ID (before deletion)
  int src_pin_index; // index in node->outputs
  int dst_node_id; // old node ID (before deletion)
  int dst_pin_index; // index in node->inputs
};

// ============================================================================
// AddNodeCommand
// ============================================================================

/**
 * @brief Command to add a node to the graph.
 *
 * Stores the node as serialized JSON to support undo without
 * keeping the original unique_ptr alive.
 */
class AddNodeCommand : public ICommand {
public:
  /**
   * @brief Create a command to add a node.
   *
   * @param node Node to add (ownership transferred on execute)
   */
  explicit AddNodeCommand(std::unique_ptr<Node> node);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  [[nodiscard]] std::string description() const override;

private:
  std::unique_ptr<Node> node_; // Valid before first execute
  nlohmann::json node_data_; // Serialized after execute (for redo)
  int added_node_id_{0}; // ID of the added node
};

// ============================================================================
// DeleteNodesCommand
// ============================================================================

/**
 * @brief Command to delete one or more nodes and their associated links.
 */
class DeleteNodesCommand : public ICommand {
public:
  /**
   * @brief Create a command to delete nodes by ID.
   *
   * @param node_ids Set of node IDs to delete
   */
  explicit DeleteNodesCommand(std::unordered_set<int> node_ids);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  std::string description() const override;

private:
  std::unordered_set<int> node_ids_;

  // Captured state for undo
  std::vector<nlohmann::json> deleted_nodes_;
  std::vector<Link> deleted_links_;

  std::vector<SerializedLink> deleted_links_by_index_;
};

// ============================================================================
// AddLinkCommand
// ============================================================================

/**
 * @brief Command to create a link between two pins.
 */
class AddLinkCommand : public ICommand {
public:
  /**
   * @brief Create a command to add a link.
   *
   * @param start_pin_id Output pin ID
   * @param end_pin_id Input pin ID
   */
  AddLinkCommand(int start_pin_id, int end_pin_id);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  [[nodiscard]] std::string description() const override;

private:
  int start_pin_id_;
  int end_pin_id_;
  int created_link_id_{0}; // Set during execute
};

// ============================================================================
// DeleteLinksCommand
// ============================================================================

/**
 * @brief Command to delete one or more links.
 */
class DeleteLinksCommand : public ICommand {
public:
  /**
   * @brief Create a command to delete links by ID.
   */
  explicit DeleteLinksCommand(std::unordered_set<int> link_ids);

  /// To delete just one link, you can also use the constructor that takes a single link ID:
  explicit DeleteLinksCommand(int link_id);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  std::string description() const override;

private:
  std::unordered_set<int> link_ids_;
  std::vector<Link> deleted_links_; // For undo
};

// ============================================================================
// MoveNodesCommand
// ============================================================================

/**
 * @brief Command to move one or more nodes to new positions.
 */
class MoveNodesCommand : public ICommand {
public:
  struct NodePosition {
    int node_id{};
    ImVec2 old_pos;
    ImVec2 new_pos;
  };

  /**
   * @brief Create a command to move nodes.
   */
  explicit MoveNodesCommand(std::vector<NodePosition> moves);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  [[nodiscard]] std::string description() const override;

private:
  std::vector<NodePosition> moves_;
};

#endif  // NAG_EDITOR_GRAPH_COMMANDS_H
