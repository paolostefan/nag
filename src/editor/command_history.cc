#include "editor/command_history.h"

#include "spdlog/spdlog.h"

#include "engine/node_graph.h"

bool CommandHistory::execute(NodeGraph &graph, std::unique_ptr<ICommand> command) {
  if (!command) {
    spdlog::error("Cannot execute null command");
    return false;
  }

  // Try to execute the command
  if (!command->execute(graph)) {
    spdlog::warn("Command '{}' failed to execute", command->description());
    return false;
  }

  // Success: add to undo stack and clear redo stack
  undo_stack_.push_back(std::move(command));

  // Limit stack size
  if (undo_stack_.size() > kMaxHistorySize) {
    undo_stack_.pop_front();
  }

  // New command invalidates redo stack
  redo_stack_.clear();

  spdlog::debug("Executed command: {}", undo_stack_.back()->description());
  return true;
}

bool CommandHistory::undo(NodeGraph &graph) {
  if (!can_undo()) {
    return false;
  }

  auto command = std::move(undo_stack_.back());
  undo_stack_.pop_back();

  if (!command->undo(graph)) {
    spdlog::error("Failed to undo command: {}", command->description());
    // Put it back? Or leave it removed? Design choice.
    // Let's remove it (fail-safe: don't let broken commands block undo)
    return false;
  }

  redo_stack_.push_back(std::move(command));

  spdlog::debug("Undid command: {}", redo_stack_.back()->description());
  return true;
}

bool CommandHistory::redo(NodeGraph &graph) {
  if (!can_redo()) {
    return false;
  }

  auto command = std::move(redo_stack_.back());
  redo_stack_.pop_back();

  if (!command->execute(graph)) {
    spdlog::error("Failed to redo command: {}", command->description());
    return false;
  }

  undo_stack_.push_back(std::move(command));

  spdlog::debug("Redid command: {}", undo_stack_.back()->description());
  return true;
}

std::string CommandHistory::get_undo_description() const {
  if (!can_undo()) return "";
  return undo_stack_.back()->description();
}

std::string CommandHistory::get_redo_description() const {
  if (!can_redo()) return "";
  return redo_stack_.back()->description();
}
