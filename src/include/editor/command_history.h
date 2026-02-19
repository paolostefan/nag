#ifndef NAG_EDITOR_COMMAND_HISTORY_H
#define NAG_EDITOR_COMMAND_HISTORY_H

#include <deque>
#include <memory>

#include "editor/command.h"

/**
 * @brief Manages undo/redo stacks for graph editor commands.
 *
 * Maintains two stacks:
 * - Undo stack: previously executed commands that can be undone
 * - Redo stack: previously undone commands that can be re-executed
 *
 * Any new command execution clears the redo stack.
 */
class CommandHistory {
public:
  /**
   * @brief Execute a command and add it to the undo stack.
   *
   * Clears the redo stack. If the command fails to execute,
   * it is not added to the history.
   *
   * @param graph The graph to operate on
   * @param command The command to execute
   * @return true if command executed successfully, false otherwise
   */
  bool execute(NodeGraph &graph, std::unique_ptr<ICommand> command);

  /**
   * @brief Undo the most recent command.
   *
   * @param graph The graph to operate on
   * @return true if undo succeeded, false if nothing to undo
   */
  bool undo(NodeGraph &graph);

  /**
   * @brief Redo the most recently undone command.
   *
   * @param graph The graph to operate on
   * @return true if redo succeeded, false if nothing to redo
   */
  bool redo(NodeGraph &graph);

  /**
   * @brief Check if undo is available.
   */
  [[nodiscard]] bool can_undo() const noexcept {
    return !undo_stack_.empty();
  }

  /**
   * @brief Check if redo is available.
   */
  [[nodiscard]] bool can_redo() const noexcept {
    return !redo_stack_.empty();
  }

  /**
   * @brief Clear both undo and redo stacks.
   *
   * Call this when loading a new graph or resetting the editor.
   */
  void clear() noexcept {
    undo_stack_.clear();
    redo_stack_.clear();
  }

  /**
   * @brief Get description of the next undo command.
   */
  [[nodiscard]] std::string get_undo_description() const;

  /**
   * @brief Get description of the next redo command.
   */
  [[nodiscard]] std::string get_redo_description() const;

  /**
   * @brief Get number of commands in undo stack.
   */
  [[nodiscard]] size_t undo_count() const noexcept {
    return undo_stack_.size();
  }

  /**
   * @brief Get number of commands in redo stack.
   */
  [[nodiscard]] size_t redo_count() const noexcept {
    return redo_stack_.size();
  }

private:
  static constexpr size_t kMaxHistorySize = 50;

  std::deque<std::unique_ptr<ICommand> > undo_stack_;
  std::deque<std::unique_ptr<ICommand> > redo_stack_;
};

#endif  // NAG_EDITOR_COMMAND_HISTORY_H
