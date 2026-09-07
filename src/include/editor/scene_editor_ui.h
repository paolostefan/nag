#ifndef NAG_EDITOR_SCENE_EDITOR_UI_H
#define NAG_EDITOR_SCENE_EDITOR_UI_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "ImSequencer.h"
#include "imnodes.h"
#include "editor/graph_editor_ui.h"
#include "editor/graph_library_panel.h"
#include "editor/graph_scene_sync.h"
#include "editor/recent_files_manager.h"
#include "editor/scene.h"
#include "editor/scene_library.h"
#include "editor/timeline_controller.h"

class SceneEditorUI : public GraphEditorUI {
public:
  SceneEditorUI();

  void render_top_status_bar();

  ~SceneEditorUI() override = default;

protected:
  void render_ui() override;

  void display_dialogs() override;

private:
  void handle_keyboard_shortcuts();

  void render_menu_bar() override;

  void render_timeline();

  void new_scene();

  /// @brief Rebind scene-dependent modules (timeline, sync, library panel) to scene_.
  void wire_scene_dependencies();

  static void open_scene();

  void save_scene();

  static void save_scene_as();

  void save_current_graph();

  void load_graph_from_library(GraphReference &graph_ref);

  GraphReference *add_graph_in_folder(GraphFolder &folder, std::string_view graph_name = "New Graph");

  void select_first_available_graph();

  /// @brief After undo/redo, reselect when the active graph was removed by the operation.
  void refresh_graph_selection_after_history();

  void open_audio_track_dialog();

  // ── Recent Scenes ───────────────────────────────────────────────────────────
  [[nodiscard]] static std::filesystem::path get_recent_scenes_path();

  void on_graph_modified() override;

  void request_quit() override;

  // Quits the application
  void quit();

  static constexpr float kStatusMessageDuration{3.f};
  static constexpr auto kSaveSceneDialogKey{"SaveSceneDlg"};
  static constexpr auto kOpenSceneDialogKey{"OpenSceneDialogKey"};
  static constexpr auto kImportAudioDialogKey{"ImportAudioDlg"};
  static constexpr auto kRecentScenesFile{"nag_recent.json"};

  std::string current_scene_path_;
  RecentFilesManager recent_files_;

  std::unique_ptr<TimelineController> timeline_controller_;

  /// @brief The currently loaded scene.
  /// The editor operates on this scene, and it can be replaced when loading a new scene or creating a new one.
  std::unique_ptr<Scene> scene_;

  /// The scene library manages loading/saving scenes and graphs, and provides a list of available graphs for the library panel.
  std::unique_ptr<SceneLibrary> scene_library_;

  /// @brief Graph<->scene data movement.
  std::unique_ptr<GraphSceneSync> graph_scene_sync_;

  /// @brief Graph library dock panel.
  std::unique_ptr<GraphLibraryPanel> graph_library_panel_;

  /// The currently selected graph reference from the library panel. This is used to determine which graph to load into the editor when a graph is selected.
  GraphReference *current_graph_ref{nullptr};

  int selected_segment_{-1};
  int current_frame_{0};
  int first_frame{0};

  /// True when quitting while scene is dirty — triggers confirmation dialog
  std::atomic<bool> quit_requested_{false};

};

#endif // NAG_EDITOR_SCENE_EDITOR_UI_H
