#include "editor/scene_editor_ui.h"

#include <IconsFontAwesome6.h>
#include <imgui_internal.h>

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "imnodes.h"
#include "spdlog/spdlog.h"

SceneEditorUI::SceneEditorUI() : GraphEditorUI("Scene Editor", 1280, 800) {
  scene_library_ = std::make_unique<SceneLibrary>("./scenes");

  new_scene();

  // Start with at least a "Default" folder and a default graph in it for better UX
  const auto *default_folder = scene_->add_folder(nullptr, "Default");
  [[maybe_unused]] auto *default_graph = scene_->add_graph(default_folder->id, "Unnamed graph");

  // Add a time node and an output node to the default graph to avoid starting with an empty graph
  time_node = reinterpret_cast<TimeNode *>(spawn_node(NodeType::Time, ImVec2(100, 100)));
  output_node = reinterpret_cast<OutputNode *>(spawn_node(NodeType::Output, ImVec2(300, 100)));
}

void SceneEditorUI::render_top_status_bar() {
  // According to https://github.com/ocornut/imgui/issues/3518#issuecomment-918186716
  // this is the right way to implement a statusbar.
  constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_MenuBar;
  float height = ImGui::GetFrameHeight();
  if (ImGui::BeginViewportSideBar("##MainStatusBar", nullptr, ImGuiDir_Up, height, window_flags)) {
    if (ImGui::BeginMenuBar()) {
      ImGui::TextUnformatted(scene_->name.c_str());

      // Print the status message right after the scene title
      print_status_message();

      ImGui::EndMenuBar();
    }
    ImGui::End();
  }
}

void SceneEditorUI::render_ui() {

  render_menu_bar();

  render_top_status_bar();

  render_node_editor();

  render_node_props();

  render_graph_library_panel();
  render_timeline();
}

void SceneEditorUI::display_dialogs() {
  GraphEditorUI::display_dialogs();

  constexpr ImVec2 kDialogSize{600.f, 400.f};

  // ── File Dialog: Open scene ──────────────────────────────────────────────
  if (ImGuiFileDialog::Instance()->Display(
    kOpenSceneDialogKey, ImGuiWindowFlags_NoCollapse, kDialogSize)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const auto scene_path = ImGuiFileDialog::Instance()->GetFilePathName();
      scene_ = SceneLibrary::load_scene(scene_path);
      if (scene_) {
        current_scene_path_ = scene_path;
        reset_graph();
        set_status_message("Scene loaded: " + scene_->name);
      } else {
        spdlog::error("Failed to load scene: {}", scene_path);
        set_status_message("Failed to load scene");

        // Resort to a new scene to avoid leaving the editor in a broken state
        new_scene();
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  // ── File Dialog: Save scene ──────────────────────────────────────────────
  if (ImGuiFileDialog::Instance()->Display(
    kSaveSceneDialogKey, ImGuiWindowFlags_NoCollapse, kDialogSize)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      current_scene_path_ = ImGuiFileDialog::Instance()->GetFilePathName();
      save_scene();
    }
    ImGuiFileDialog::Instance()->Close();
  }
}

void SceneEditorUI::render_folder_tree(const std::vector<std::unique_ptr<GraphFolder> > &folders) {
  for (auto &folder: folders) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (const bool has_children = !folder->children.empty(); !has_children && folder->graphs.empty()) {
      flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const bool opened = ImGui::TreeNodeEx(folder->id.c_str(), flags, "%s", folder->name.c_str());

    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem(ICON_FA_FOLDER_PLUS "  Add Subfolder")) {
        [[maybe_unused]] auto *subfolder = scene_->add_folder(folder->id, "New Subfolder");
      }
      if (ImGui::MenuItem(ICON_FA_DIAGRAM_PROJECT "  Add Graph Here")) {
        [[maybe_unused]] auto *subgraph = scene_->add_graph(folder->id, "New Graph");
      }

      if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename")) {
        rename_target_ = RenameTargetFolder;
        rename_target_id_ = folder->id.c_str();
        is_renaming_.store(true, std::memory_order_release);
      }

      ImGui::Separator();

      if (ImGui::MenuItem(ICON_FA_TRASH "  Delete Folder")) {
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
        constexpr ImGuiTreeNodeFlags kGraphFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(graph_ref.id.c_str(), kGraphFlags, "%s", graph_ref.name.c_str())) {
          if (ImGui::IsItemClicked()) {
            load_graph_from_library(graph_ref.id);
          }

          if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename")) {
              rename_target_ = RenameTargetGraph;
              rename_target_id_ = graph_ref.id.c_str();
              is_renaming_.store(true, std::memory_order_release);
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_TRASH "  Delete")) {
              scene_->remove_graph(graph_ref.id);
            }
            ImGui::EndPopup();
          }
          ImGui::TreePop();
        }
      }

      render_folder_tree(folder->children);
      ImGui::TreePop();
    } // if opened
  } // for each folder
}

void SceneEditorUI::render_menu_bar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
        new_scene();
      }
      if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Open Scene...", "Ctrl+O")) {
        open_scene();
      }
      ImGui::Separator();
      if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save Scene", "Ctrl+S")) {
        save_scene();
      }
      if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save Scene As...", "Ctrl+Shift+S")) {
        save_scene_as();
      }

      ImGui::Separator();

      if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save Current Graph", "Ctrl+G")) {
        save_current_graph();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem(ICON_FA_ARROW_ROTATE_LEFT "  Undo", "Ctrl+Z")) {
        command_history.undo(graph);
        node_pos_refresh = true;
      }
      if (ImGui::MenuItem(ICON_FA_ARROW_ROTATE_RIGHT "  Redo", "Ctrl+Y")) {
        command_history.redo(graph);
        node_pos_refresh = true;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      // Toggle preview window.
      if (ImGui::MenuItem(ICON_FA_DISPLAY "  Preview Window",
                          nullptr,
                          preview_window.is_open())) {
        if (preview_window.is_open()) {
          preview_window.close();
        } else {
          preview_window.open(window, gl_context);
        }
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void SceneEditorUI::render_graph_library_panel() {
  if (ImGui::Begin("Graph Library")) {
    if (ImGui::Button(ICON_FA_FOLDER_PLUS "  New folder")) {
      [[maybe_unused]] auto *folder = scene_->add_folder(nullptr, "New Folder");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_DIAGRAM_PROJECT "  New graph")) {
      const GraphFolder *folder = nullptr;
      if (!scene_->root_folders.empty()) {
        folder = scene_->root_folders[0].get();
      }
      if (folder) {
        [[maybe_unused]] auto *ref = scene_->add_graph(folder->id, "New Graph");
      }
    }

    ImGui::Separator();

    render_folder_tree(scene_->root_folders);
  }

  ImGui::End();

  if (is_renaming_.load(std::memory_order_acquire)) {
    ImGui::OpenPopup("RenamePopup");
  }

  if (ImGui::BeginPopup("RenamePopup")) {
    static char name_buffer[256];

    // Focus the text input when the popup opens
    if (ImGui::IsWindowAppearing()) {
      ImGui::SetKeyboardFocusHere();
      // Pre-fill the buffer with the current name
      switch (rename_target_) {
        case RenameTargetScene:
          strncpy(name_buffer, scene_->name.c_str(), sizeof(name_buffer));
          break;
        case RenameTargetFolder: {
          if (const auto *folder = scene_->find_folder(rename_target_id_)) {
            strncpy(name_buffer, folder->name.c_str(), sizeof(name_buffer));
          }
          break;
        }
        case RenameTargetGraph: {
          if (const auto *graph = scene_->find_graph(rename_target_id_)) {
            strncpy(name_buffer, graph->name.c_str(), sizeof(name_buffer));
          }
          break;
        }
        default:
          name_buffer[0] = '\0';
          break;
      }
    }

    ImGui::InputText("##renameNewName", name_buffer, sizeof(name_buffer));

    ImGui::SameLine();

    ImGui::BeginDisabled(strlen(name_buffer) == 0);
    if (ImGui::Button("OK")) {
      const std::string new_name(name_buffer);
      switch (rename_target_) {
        case RenameTargetScene:
          scene_->name = new_name;
          break;
        case RenameTargetFolder:
          scene_->rename_folder(rename_target_id_, new_name);
          break;
        case RenameTargetGraph:
          scene_->rename_graph(rename_target_id_, new_name);
          break;
        default:
          break;
      }
      ImGui::CloseCurrentPopup();
      is_renaming_.store(false, std::memory_order_release);
    }
    ImGui::EndDisabled();

    ImGui::EndPopup();
  }
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

void SceneEditorUI::Get(const int index, int **start, int **end, int *type, unsigned int *color) {
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
  const TimelineSegment segment(0, 100, "");
  scene_->add_timeline_segment(segment);
}

void SceneEditorUI::Del(const int index) {
  if (index >= 0 && index < static_cast<int>(scene_->timeline.size())) {
    scene_->remove_timeline_segment(static_cast<size_t>(index));
  }
}

void SceneEditorUI::Duplicate(const int index) {
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
  current_scene_path_.clear();
  reset_graph();
  set_status_message("New scene created");
}

void SceneEditorUI::open_scene() {
  IGFD::FileDialogConfig cfg;
  cfg.path = "./scenes/";
  ImGuiFileDialog::Instance()->OpenDialog(kOpenSceneDialogKey, "Open Scene", ".nagscene", cfg);
}

void SceneEditorUI::save_scene() {
  if (current_scene_path_.empty()) {
    save_scene_as();
    return;
  }

  if (SceneLibrary::save_scene(*scene_, current_scene_path_)) {
    scene_->pristine = true;
    set_status_message("Scene saved");
  }
}

void SceneEditorUI::save_scene_as() {
  IGFD::FileDialogConfig cfg;
  cfg.path = ".";
  ImGuiFileDialog::Instance()->OpenDialog(kSaveSceneDialogKey, "Save Scene As", ".nagscene", cfg);
}

void SceneEditorUI::save_current_graph() {
  if (scene_->root_folders.empty()) {
    [[maybe_unused]] auto *folder = scene_->add_folder(nullptr, "Default");
  }

  const auto &folder = scene_->root_folders[0];

  if (const auto graph_ref =
        scene_->add_graph(folder->id, "Graph " + std::to_string(folder->graphs.size()));
    graph_ref && scene_library_->save_graph(graph, *graph_ref)) {
    set_status_message("Graph saved: " + graph_ref->name);
  }
}

void SceneEditorUI::load_graph_from_library(const std::string &graph_id) {
  const GraphReference *graph_ref = scene_->find_graph(graph_id);
  if (!graph_ref) {
    spdlog::error("Graph not found: {}", graph_id);
    return;
  }

  if (const auto loaded_graph = scene_library_->load_graph(*graph_ref)) {
    graph = std::move(*loaded_graph);
    node_pos_refresh = true;

    // Find time and output nodes in the loaded graph
    time_node = nullptr;
    output_node = nullptr;
    for (const auto &node: graph.nodes) {
      if (auto *t = dynamic_cast<TimeNode *>(node.get())) {
        time_node = t;
      }
      if (auto *o = dynamic_cast<OutputNode *>(node.get())) {
        output_node = o;
      }
    }
    if (!output_node) {
      spdlog::warn("Graph '{}' has no output node", graph_ref->name);
    }

    set_status_message("Loaded: " + graph_ref->name);
  }
}
