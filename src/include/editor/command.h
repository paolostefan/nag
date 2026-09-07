#ifndef NAG_EDITOR_COMMAND_H
#define NAG_EDITOR_COMMAND_H

#include <memory>
#include <string>

struct NodeGraph;
class Scene;

/**
 * @brief Base interface for undoable commands.
 *
 * All editor actions that should support undo/redo must implement this interface.
 * Commands capture all state needed to reverse their action.
 *
 * Two targeting modes exist:
 * - Graph commands operate on a @c NodeGraph and implement @c execute/undo(NodeGraph&).
 * - Scene commands operate on the @c Scene (folders, graphs, timeline, audio)
 *   and set @c targets_scene() to true and implement @c execute/undo(Scene&).
 * The @c CommandHistory dispatches to the correct target automatically.
 */
class ICommand {
public:
  virtual ~ICommand() = default;

  /**
   * @brief True if this command operates on the Scene instead of a NodeGraph.
   */
  [[nodiscard]] virtual bool targets_scene() const {
    return false;
  }

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
   * @brief Execute the command against the Scene.
   *
   * Scene commands override this; graph commands keep the default (returns false).
   */
  [[nodiscard]] virtual bool execute(Scene &scene) {
    (void)scene;
    return false;
  }

  /**
   * @brief Undo the command, reverting the scene to its previous state.
   *
   * Scene commands override this; graph commands keep the default (returns false).
   */
  [[nodiscard]] virtual bool undo(Scene &scene) {
    (void)scene;
    return false;
  }

  /**
   * @brief Get a human-readable description of this command.
   *
   * Used for debugging and potentially for showing undo history to the user.
   */
  [[nodiscard]] virtual std::string description() const = 0;
};


#endif // NAG_EDITOR_COMMAND_H
