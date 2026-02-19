#ifndef NAG_EDITOR_GRAPH_COMMANDS_H
#define NAG_EDITOR_GRAPH_COMMANDS_H

#include <memory>
#include <unordered_set>
#include <vector>

#include "nlohmann/json.hpp"

#include "editor/command.h"
#include "engine/node.h"
#include "engine/node_graph.h"

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
  uint32_t added_node_id_{0}; // ID of the added node
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
  explicit DeleteNodesCommand(std::unordered_set<uint32_t> node_ids);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  std::string description() const override;

private:
  std::unordered_set<uint32_t> node_ids_;

  // Captured state for undo
  std::vector<nlohmann::json> deleted_nodes_;
  std::vector<Link> deleted_links_;
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
  AddLinkCommand(uint32_t start_pin_id, uint32_t end_pin_id);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  [[nodiscard]] std::string description() const override;

private:
  uint32_t start_pin_id_;
  uint32_t end_pin_id_;
  uint32_t created_link_id_{0}; // Set during execute
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
  explicit DeleteLinksCommand(std::unordered_set<uint32_t> link_ids);

  bool execute(NodeGraph &graph) override;

  bool undo(NodeGraph &graph) override;

  std::string description() const override;

private:
  std::unordered_set<uint32_t> link_ids_;
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
    uint32_t node_id{};
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
