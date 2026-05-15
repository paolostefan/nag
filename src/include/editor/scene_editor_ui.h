#ifndef NAG_EDITOR_SCENE_EDITOR_UI_H
#define NAG_EDITOR_SCENE_EDITOR_UI_H

#include <memory>
#include <string>

#include "ImSequencer.h"
#include "imnodes.h"
#include "editor/graph_editor_ui.h"
#include "editor/scene.h"
#include "editor/scene_library.h"

class SceneEditorUI : public GraphEditorUI, public ImSequencer::SequenceInterface {
public:
  SceneEditorUI();

  void render_top_status_bar();

  ~SceneEditorUI() override = default;

  [[nodiscard]] int GetFrameMin() const override { return 0; }
  [[nodiscard]] int GetFrameMax() const override { return scene_->total_frames; }
  [[nodiscard]] int GetItemCount() const override { return static_cast<int>(scene_->timeline.size()); }
  [[nodiscard]] int GetItemTypeCount() const override { return 1; }
  [[nodiscard]] const char *GetItemTypeName(int type_index) const override { return "Graph"; }

  void Get(int index, int **start, int **end, int *type, unsigned int *color) override;

  void Add(int type) override;

  void Del(int index) override;

  void Duplicate(int index) override;

protected:
  void render_ui() override;


  void display_dialogs() override;

private:
  void render_menu_bar();

  void render_graph_library_panel();

  void render_timeline();

  void new_scene();

  bool load_scene(const std::string &path);

  static void open_scene();

  void save_scene();

  static void save_scene_as();

  void save_current_graph();

  void load_graph_from_library(const std::string &graph_id);

  void render_folder_tree(const std::vector<std::unique_ptr<GraphFolder>> &folders);

  /// @brief The currently loaded scene.
  /// The editor operates on this scene, and it can be replaced when loading a new scene or creating a new one.
  std::unique_ptr<Scene> scene_;

  /// The scene library manages loading/saving scenes and graphs, and provides a list of available graphs for the library panel.
  std::unique_ptr<SceneLibrary> scene_library_;

  std::string current_scene_path_;

  int selected_segment_{-1};
  int expanded_folders_[64]{};
  int expanded_folder_count_{0};

  static constexpr float kStatusMessageDuration{3.f};
  static constexpr auto kSaveSceneDialogKey{"SaveSceneDlg"};
  static constexpr auto kOpenSceneDialogKey{"OpenSceneDialogKey"};

  ImNodesEditorContext *editor_context_{nullptr};

  /// Used to prevent multiple rename popups from opening simultaneously
  std::atomic<bool> is_renaming_{false};

  /// Tracks what type of item is currently being renamed (scene, folder, or graph)
  enum {
    RenameTargetNone,
    RenameTargetScene,
    RenameTargetFolder,
    RenameTargetGraph
  } rename_target_{RenameTargetNone};

  /// Stores the ID of the item being renamed (folder ID or graph ID)
  const char *rename_target_id_{nullptr};

};

#endif // NAG_EDITOR_SCENE_EDITOR_UI_H
