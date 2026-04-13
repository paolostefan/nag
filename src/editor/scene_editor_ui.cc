#include "editor/scene_editor_ui.h"

#include <functional>

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "imnodes.h"
#include "spdlog/spdlog.h"

#include "engine/nodes/generator_nodes.h"

SceneEditorUI::SceneEditorUI() : GraphEditorUI("Scene Editor", 1280, 800) {
  scene_ = std::make_unique<Scene>("New Scene");
  scene_library_ = std::make_unique<SceneLibrary>("./scenes");

  scene_->add_folder(std::nullopt, "Effects");
  scene_->add_folder(std::nullopt, "Generators");
}

void SceneEditorUI::render_ui() {
  GraphEditorUI::render_ui();

  render_graph_library_panel();
  render_timeline();


  if (!status_message_.empty() && ImGui::GetTime() - status_message_time_ < kStatusMessageDuration) {
    ImGui::Begin("Status");
    ImGui::Text("%s", status_message_.c_str());
    ImGui::End();
  } else {
    status_message_.clear();
  }
}

void SceneEditorUI::render_menu_bar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
        new_scene();
      }
      if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
        open_scene();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
        save_scene();
      }
      if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
        save_scene_as();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Save Current Graph", "Ctrl+G")) {
        save_current_graph();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
        command_history.undo(graph);
        node_pos_refresh = true;
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
        command_history.redo(graph);
        node_pos_refresh = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

void SceneEditorUI::render_graph_library_panel() {
  ImGui::Begin("Graph Library");

  if (ImGui::Button("+ Folder")) {
    [[maybe_unused]] auto *folder = scene_->add_folder(std::nullopt, "New Folder");
  }
  ImGui::SameLine();
  if (ImGui::Button("+ Graph")) {
    GraphFolder *folder = nullptr;
    if (!scene_->root_folders.empty()) {
      folder = scene_->root_folders[0].get();
    }
    if (folder) {
      [[maybe_unused]] auto *ref = scene_->add_graph(folder->id, "New Graph", graph);
    }
  }

  ImGui::Separator();

  std::function<void(std::vector<std::unique_ptr<GraphFolder> > &)> render_folder_tree;
  render_folder_tree = [this, &render_folder_tree](std::vector<std::unique_ptr<GraphFolder> > &folders) {
    for (auto &folder: folders) {
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
      if (const bool has_children = !folder->children.empty(); !has_children && folder->graphs.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
      }

      const bool opened = ImGui::TreeNodeEx(folder->id.c_str(), flags, "%s", folder->name.c_str());

      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Add Subfolder")) {
          scene_->add_folder(folder->id, "New Subfolder");
        }
        if (ImGui::MenuItem("Add Graph Here")) {
          scene_->add_graph(folder->id, "New Graph", graph);
        }
        if (ImGui::MenuItem("Delete Folder")) {
          scene_->remove_folder(folder->id);
          ImGui::EndPopup();
          if (opened)
            ImGui::TreePop();
          continue;
        }
        ImGui::EndPopup();
      }

      if (opened) {
        for (auto &graph_ref: folder->graphs) {
          ImGuiTreeNodeFlags graph_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
          if (ImGui::TreeNodeEx(graph_ref.id.c_str(), graph_flags, "%s", graph_ref.name.c_str())) {
            if (ImGui::IsItemClicked()) {
              load_graph_from_library(graph_ref.id);
            }
            if (ImGui::BeginPopupContextItem()) {
              if (ImGui::MenuItem("Rename")) {
              }
              if (ImGui::MenuItem("Delete")) {
                scene_->remove_graph(graph_ref.id);
              }
              ImGui::EndPopup();
            }
            ImGui::TreePop();
          }
        }

        render_folder_tree(folder->children);
        ImGui::TreePop();
      }
    }
  };

  render_folder_tree(scene_->root_folders);

  ImGui::End();
}

void SceneEditorUI::render_timeline() {
  ImGui::Begin("Timeline");

  ImGui::SetNextWindowSize(ImVec2(ImGui::GetContentRegionAvail().x, 150), ImGuiCond_Always);
  ImGui::BeginChild("TimelineRegion", ImVec2(0, 150), true);

  if (ImGui::CollapsingHeader("Timeline", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Button("Add Segment")) {
      TimelineSegment segment(0, 100, "");
      scene_->add_timeline_segment(segment);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Selected") && selected_segment_ >= 0) {
      scene_->remove_timeline_segment(static_cast<size_t>(selected_segment_));
      selected_segment_ = -1;
    }

    ImGui::Separator();

    static int current_frame = 0;
    static bool expanded = true;
    ImSequencer::Sequencer(this, &current_frame, &expanded, &selected_segment_, nullptr,
                           ImSequencer::SEQUENCER_EDIT_ALL);

    if (selected_segment_ >= 0 && selected_segment_ < static_cast<int>(scene_->timeline.size())) {
      const auto &segment = scene_->timeline[static_cast<size_t>(selected_segment_)];
      ImGui::Text("Selected: frames %d - %d, graph: %s", segment.frame_start, segment.frame_end,
                  segment.graph_id.empty() ? "(none)" : segment.graph_id.c_str());
    }
  }

  ImGui::EndChild();
  ImGui::End();
}

void SceneEditorUI::Get(int index, int **start, int **end, int *type, unsigned int *color) {
  if (index < 0 || index >= static_cast<int>(scene_->timeline.size())) {
    return;
  }

  const TimelineSegment &segment = scene_->timeline[static_cast<size_t>(index)];
  if (start)
    *start = const_cast<int *>(&segment.frame_start);
  if (end)
    *end = const_cast<int *>(&segment.frame_end);
  if (type)
    *type = 0;
  if (color)
    *color = segment.color;
}

void SceneEditorUI::Add(int type) {
  TimelineSegment segment(0, 100, "");
  scene_->add_timeline_segment(segment);
}

void SceneEditorUI::Del(int index) {
  if (index >= 0 && index < static_cast<int>(scene_->timeline.size())) {
    scene_->remove_timeline_segment(static_cast<size_t>(index));
  }
}

void SceneEditorUI::Duplicate(int index) {
  if (index >= 0 && index < static_cast<int>(scene_->timeline.size())) {
    const TimelineSegment &original = scene_->timeline[static_cast<size_t>(index)];
    TimelineSegment copy = original;
    copy.frame_start = original.frame_end;
    copy.frame_end = original.frame_end + (original.frame_end - original.frame_start);
    scene_->add_timeline_segment(copy);
  }
}

void SceneEditorUI::new_scene() {
  scene_ = std::make_unique<Scene>("New Scene");
  scene_->add_folder(std::nullopt, "Effects");
  scene_->add_folder(std::nullopt, "Generators");
  current_scene_path_.clear();
  reset_graph();
  status_message_ = "New scene created";
  status_message_time_ = ImGui::GetTime();
}

void SceneEditorUI::open_scene() {
  IGFD::FileDialogConfig cfg;
  cfg.path = ".";
  ImGuiFileDialog::Instance()->OpenDialog(kOpenSceneDialogKey, "Open Scene", ".nagscene", cfg);
}

void SceneEditorUI::save_scene() {
  if (current_scene_path_.empty()) {
    save_scene_as();
    return;
  }

  if (scene_library_->save_scene(*scene_, current_scene_path_)) {
    scene_->pristine = true;
    status_message_ = "Scene saved";
    status_message_time_ = ImGui::GetTime();
  }
}

void SceneEditorUI::save_scene_as() {
  IGFD::FileDialogConfig cfg;
  cfg.path = ".";
  ImGuiFileDialog::Instance()->OpenDialog(kSaveSceneDialogKey, "Save Scene As", ".nagscene", cfg);
}

void SceneEditorUI::save_current_graph() {
  if (scene_->root_folders.empty()) {
    scene_->add_folder(std::nullopt, "Default");
  }

  auto &folder = scene_->root_folders[0];
  auto graph_ref = scene_->add_graph(folder->id, "Graph " + std::to_string(folder->graphs.size()), graph);

  if (graph_ref && scene_library_->save_graph(graph, *graph_ref)) {
    status_message_ = "Graph saved: " + graph_ref->name;
    status_message_time_ = ImGui::GetTime();
  }
}

void SceneEditorUI::load_graph_from_library(const std::string &graph_id) {
  const GraphReference *graph_ref = scene_->find_graph(graph_id);
  if (!graph_ref) {
    spdlog::error("Graph not found: {}", graph_id);
    return;
  }

  auto loaded_graph = scene_library_->load_graph(*graph_ref);
  if (loaded_graph) {
    graph = std::move(*loaded_graph);
    node_pos_refresh = true;
    status_message_ = "Loaded: " + graph_ref->name;
    status_message_time_ = ImGui::GetTime();
  }
}
