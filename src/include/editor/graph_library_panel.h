#ifndef NAG_EDITOR_GRAPH_LIBRARY_PANEL_H
#define NAG_EDITOR_GRAPH_LIBRARY_PANEL_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "editor/scene.h"

/**
 * @brief Renders and owns the state of the "Graph Library" dock panel.
 *
 * Draws the folder tree, the New folder/New graph buttons, the rename popup,
 * and the delete-graph confirmation modal. Owns the rename/delete selection
 * state internally. Emits high-level events to the editor through @ref Callbacks.
 */
class GraphLibraryPanel {
public:
  struct Callbacks {
    /// @brief Open a graph into the editor.
    std::function<void(GraphReference &)> on_open_graph;

    /// @brief Create a new graph inside a folder; returns the new reference.
    std::function<GraphReference *(GraphFolder &)> on_add_graph;

    /// @brief Add a new root folder.
    std::function<void()> on_add_folder;

    /// @brief React after a graph is actually deleted (editor may reselect).
    std::function<void()> on_graph_deleted;
  };

  GraphLibraryPanel(Scene &scene, GraphReference *&current_graph,
                    Callbacks callbacks);

  /// @brief Render the panel and any open popups/modals.
  void render();

private:
  enum RenameTarget {
    RenameTargetNone,
    RenameTargetScene,
    RenameTargetFolder,
    RenameTargetGraph
  };

  void render_folder_tree(const std::vector<std::unique_ptr<GraphFolder> > &folders);
  void render_rename_popup();

  Scene &scene_;
  GraphReference *&current_graph_;

  Callbacks callbacks_;

  /// ID of the item being renamed (folder ID or graph ID)
  std::string rename_target_id_;
  RenameTarget rename_target_{RenameTargetNone};
  std::atomic<bool> is_renaming_{false};

  /// Graph ID pending confirmation for deletion
  std::string pending_delete_graph_id_;
};

#endif // NAG_EDITOR_GRAPH_LIBRARY_PANEL_H
