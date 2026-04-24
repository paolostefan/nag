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

  void render_menu_bar() override;

  void display_dialogs() override;

private:
  void render_graph_library_panel();

  void render_timeline();

  void new_scene();

  static void open_scene();

  void save_scene();

  static void save_scene_as();

  void save_current_graph();

  void load_graph_from_library(const std::string &graph_id);

  void render_folder_tree(const std::vector<std::unique_ptr<GraphFolder>> &folders);

  std::unique_ptr<Scene> scene_;
  std::unique_ptr<SceneLibrary> scene_library_;

  std::string current_scene_path_;
  std::string status_message_;
  double status_message_time_{0.f};

  int selected_segment_{-1};
  int expanded_folders_[64]{};
  int expanded_folder_count_{0};

  static constexpr float kStatusMessageDuration{3.f};
  static constexpr auto kSaveSceneDialogKey{"SaveSceneDlg"};
  static constexpr auto kOpenSceneDialogKey{"OpenSceneDialogKey"};

  ImNodesEditorContext *editor_context_{nullptr};
};

#endif // NAG_EDITOR_SCENE_EDITOR_UI_H
