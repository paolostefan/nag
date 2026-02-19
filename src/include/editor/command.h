#ifndef NAG_EDITOR_COMMAND_H
#define NAG_EDITOR_COMMAND_H

#include <memory>
#include <string>

struct NodeGraph;

/**
 * @brief Base interface for undoable commands.
 *
 * All editor actions that should support undo/redo must implement this interface.
 * Commands capture all state needed to reverse their action.
 */
class ICommand {
public:
  virtual ~ICommand() = default;

  /**
   * @brief Execute the command (or re-execute after undo).
   *
   * @param graph The graph to operate on
   * @return true if successful, false otherwise
   */
  [[nodiscard]] virtual bool execute(NodeGraph &graph) = 0;

  /**
   * @brief Undo the command, reverting the graph to its previous state.
   *
   * @param graph The graph to operate on
   * @return true if successful, false otherwise
   */
  [[nodiscard]] virtual bool undo(NodeGraph &graph) = 0;

  /**
   * @brief Get a human-readable description of this command.
   *
   * Used for debugging and potentially for showing undo history to the user.
   */
  [[nodiscard]] virtual std::string description() const = 0;
};


#endif // NAG_EDITOR_COMMAND_H
