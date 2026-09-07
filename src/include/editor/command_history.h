#ifndef NAG_EDITOR_COMMAND_HISTORY_H
#define NAG_EDITOR_COMMAND_HISTORY_H

#include <deque>
#include <memory>

#include "editor/command.h"

class Scene;

/**
 * @brief Manages undo/redo stacks for editor commands.
 *
 * Maintains two stacks:
 * - Undo stack: previously executed commands that can be undone
 * - Redo stack: previously undone commands that can be re-executed
 *
 * Commands target either the NodeGraph (graph commands) or the bound Scene
 * (scene commands, detected via @c ICommand::targets_scene()). Both kinds live
 * in the same stacks; the graph-centric entry points dispatch automatically to
 * the bound scene.
 *
 * Any new command execution clears the redo stack.
 */
class CommandHistory {
public:
  /**
   * @brief Bind the Scene that scene-commands operate on.
   *
   * Must be called before executing/scene-targeted commands. Undoing a scene
   * command without a bound scene is an error and returns false.
   */
  void bind_scene(Scene &scene) {
    scene_ = &scene;
  }

  /**
   * @brief Execute a command and add it to the undo stack.
   *
   * Clears the redo stack. If the command fails to execute,
   * it is not added to the history.
   *
   * Scene commands are dispatched to the bound scene (see @ref bind_scene).
   *
   * @param graph The graph to operate on
   * @param command The command to execute
   * @return true if command executed successfully, false otherwise
   */
  bool execute(NodeGraph &graph, std::unique_ptr<ICommand> command);

  /**
   * @brief Execute a scene-targeted command against the bound scene.
   *
   * Convenience for UI code that only has the scene. Returns false (with an
   * error log) if the command does not target the scene or no scene is bound.
   *
   * @param scene The scene to operate on
   * @param command The scene command to execute
   * @return true if command executed successfully, false otherwise
   */
  bool execute(Scene &scene, std::unique_ptr<ICommand> command);

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
   * @brief Remove graph-targeted commands from both stacks while keeping
   *        scene-targeted commands (folders, timeline, audio tracks).
   *
   * Call this when loading/switching a graph file, so undo history for the
   * scene layout survives graph swaps but stale graph commands do not.
   */
  void clear_graph_commands() noexcept {
    std::erase_if(undo_stack_, [](const std::unique_ptr<ICommand> &cmd) {
      return !cmd->targets_scene();
    });
    std::erase_if(redo_stack_, [](const std::unique_ptr<ICommand> &cmd) {
      return !cmd->targets_scene();
    });
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

  /// @brief Push a successfully executed command, trimming history and
  ///        clearing the redo stack.
  void commit(std::unique_ptr<ICommand> command);

  /// @brief The scene that scene-commands operate on (nullptr if not bound).
  Scene *scene_{nullptr};

  std::deque<std::unique_ptr<ICommand> > undo_stack_;
  std::deque<std::unique_ptr<ICommand> > redo_stack_;
};

#endif  // NAG_EDITOR_COMMAND_HISTORY_H
