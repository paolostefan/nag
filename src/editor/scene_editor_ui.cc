#include "editor/scene_editor_ui.h"

#include <IconsFontAwesome6.h>
#include <imgui_internal.h>

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "imnodes.h"
#include "spdlog/spdlog.h"

SceneEditorUI::SceneEditorUI() : GraphEditorUI("Scene Editor", 1280, 800) {
  scene_library_ = std::make_unique<SceneLibrary>();

  // Create a new empty scene
  new_scene();

  // Start with at least a default folder and a default graph in it for better UX
  auto *default_folder = scene_->add_folder(nullptr, "Root");
  current_graph_ref = add_graph_in_folder(*default_folder);
  current_graph_ref->dirty = true;

  // Add a time node and an output node to the default graph to avoid starting with an empty graph
  time_node = reinterpret_cast<TimeNode *>(spawn_node(NodeType::Time, ImVec2(100, 100)));
  output_node = reinterpret_cast<OutputNode *>(spawn_node(NodeType::Output, ImVec2(300, 100)));

  // Enable history after initial setup to avoid polluting the command history with setup actions
  history_enabled.store(true, std::memory_order_release);

  // Save initial graph state
  if (current_graph_ref) {
    scene_->graph_data[current_graph_ref->id] = JsonGraphSerializer::serialize_graph(graph);
    current_graph_ref->dirty = false;
  }
}

void SceneEditorUI::render_ui() {
  handle_keyboard_shortcuts();

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

        // Auto-select the first graph in the library if it exists
        if (!scene_->root_folders.empty() && !scene_->root_folders[0]->graphs.empty()) {
          load_graph_from_library(scene_->root_folders[0]->graphs[0]);
        }
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

  // ── File Dialog: Import Audio Track ──────────────────────────────────────
  if (ImGuiFileDialog::Instance()->Display(
    kImportAudioDialogKey, ImGuiWindowFlags_NoCollapse, kDialogSize)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const auto file_path = ImGuiFileDialog::Instance()->GetFilePathName();
      try {
        AudioTrack track{std::filesystem::path(file_path)};
        const size_t idx = scene_->add_audio_track(std::move(track));
        const TimelineSegment segment(0, 100, static_cast<int>(idx));
        scene_->add_timeline_segment(segment);
        set_status_message("Audio track imported: " + track.title);
      } catch (const std::exception &e) {
        spdlog::error("Failed to import audio track: {}", e.what());
        set_status_message("Failed to import audio track");
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  // ── Confirm Delete Graph ──────────────────────────────────────────────────
  if (!pending_delete_graph_id_.empty()) {
    ImGui::OpenPopup("Delete Graph##Modal");
    // ImGui::SetNextWindowSize(ImVec2(300, 0));
    if (ImGui::BeginPopupModal("Delete Graph##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      const auto *graph = scene_->find_graph(pending_delete_graph_id_);
      ImGui::Text("Delete graph \"%s\"?", graph ? graph->name.c_str() : "(unknown)");
      ImGui::Separator();

      if (ImGui::Button("Delete", ImVec2(100, 0))) {
        scene_->remove_graph(pending_delete_graph_id_);
        pending_delete_graph_id_.clear();
        select_first_available_graph();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(100, 0))) {
        pending_delete_graph_id_.clear();
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  // ── Confirm Quit (unsaved changes) ─────────────────────────────────────────
  if (quit_requested_.load(std::memory_order_acquire)) {
    ImGui::OpenPopup("Unsaved Changes##Quit");
    if (ImGui::BeginPopupModal("Unsaved Changes##Quit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("The scene has unsaved changes.");
      ImGui::Separator();

      if (ImGui::Button("Save && Quit", ImVec2(120, 0))) {
        quit_requested_.store(false, std::memory_order_release);
        save_scene();
        running.store(false, std::memory_order_release);
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Discard && Quit", ImVec2(120, 0))) {
        quit_requested_.store(false, std::memory_order_release);
        running.store(false, std::memory_order_release);
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(100, 0))) {
        quit_requested_.store(false, std::memory_order_release);
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
}

void SceneEditorUI::select_first_available_graph() {
  std::function<GraphReference *(const std::vector<std::unique_ptr<GraphFolder> > &)> find_first;
  find_first = [&find_first](const std::vector<std::unique_ptr<GraphFolder> > &folders) -> GraphReference * {
    for (const auto &folder: folders) {
      if (!folder->graphs.empty()) {
        return &folder->graphs.front();
      }
      if (auto *found = find_first(folder->children)) {
        return found;
      }
    }
    return nullptr;
  };

  if (auto *first = find_first(scene_->root_folders)) {
    load_graph_from_library(*first);
  } else {
    current_graph_ref = nullptr;
    reset_graph();
  }
}

void SceneEditorUI::render_folder_tree(const std::vector<std::unique_ptr<GraphFolder> > &folders) {
  for (auto &folder: folders) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (folder->children.empty() && folder->graphs.empty()) {
      flags |= ImGuiTreeNodeFlags_Leaf;
    } else {
      flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    const bool opened = ImGui::TreeNodeEx(folder->id.c_str(), flags, "%s", folder->name.c_str());

    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem(ICON_FA_FOLDER_PLUS "  Add Subfolder")) {
        [[maybe_unused]] auto *subfolder = scene_->add_folder(folder->id, "New Subfolder");
      }
      if (ImGui::MenuItem(ICON_FA_DIAGRAM_PROJECT "  Add Graph Here")) {
        [[maybe_unused]] auto *subgraph = add_graph_in_folder(*folder);
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

        const bool is_selected = &graph_ref == current_graph_ref;
        if (is_selected) {
          // Highlight the selected graph
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 1.0f, 1.0f)); // Bright blue
        }

        if (ImGui::TreeNodeEx(graph_ref.id.c_str(), kGraphFlags,
                              "%s%s", graph_ref.name.c_str(), graph_ref.dirty ? " *" : "")) {
          if (ImGui::IsItemClicked() && &graph_ref != current_graph_ref) {
            load_graph_from_library(graph_ref);
          }

          if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename")) {
              rename_target_ = RenameTargetGraph;
              rename_target_id_ = graph_ref.id.c_str();
              is_renaming_.store(true, std::memory_order_release);
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_TRASH "  Delete")) {
              pending_delete_graph_id_ = graph_ref.id;
              ImGui::OpenPopup("Delete Graph##Modal");
            }
            ImGui::EndPopup();
          }
          ImGui::TreePop();
        }

        if (is_selected) {
          ImGui::PopStyleColor();
        }
      }

      render_folder_tree(folder->children);
      ImGui::TreePop();
    } // if opened
  } // for each folder
}

void SceneEditorUI::on_graph_modified() {
  if (current_graph_ref) {
    scene_->graph_data[current_graph_ref->id] = JsonGraphSerializer::serialize_graph(graph);
    current_graph_ref->dirty = false;
    scene_->dirty = true;
  }
}

void SceneEditorUI::request_quit() {
  if (quit_requested_.load(std::memory_order_acquire)) return;
  if (scene_->dirty) {
    quit_requested_.store(true, std::memory_order_release);
  } else {
    running.store(false, std::memory_order_release);
  }
}

void SceneEditorUI::quit() {
  request_quit();
}

void SceneEditorUI::handle_keyboard_shortcuts() {
  if (ImGui::IsAnyItemActive()) return;

  // Delete selected nodes/links with Delete or Backspace key
  if (ImGui::IsKeyPressed(ImGuiKey_Delete) ||
      ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
    const auto node_no = ImNodes::NumSelectedNodes();
    const auto link_no = ImNodes::NumSelectedLinks();

    // Try deleting nodes first: this will also delete their links
    if (node_no > 0) {
      // todo: this will probably delete additional links, link_no needs update
      delete_selected_nodes();
    }
    // Otherwise delete selected links
    else if (link_no > 0) {
      delete_selected_links();
    }

    if (node_no + link_no > 0) {
      set_status_message(ICON_FA_TRASH "  Deleted " + std::to_string(node_no) +
                         (node_no == 1 ? " node" : " nodes") + " and " + std::to_string(link_no) +
                         (link_no == 1 ? " link" : " links"));
    }
  }

  if (!ImGui::IsKeyDown(ImGuiMod_Ctrl)) return;

  if (ImGui::IsKeyPressed(ImGuiKey_N)) {
    new_scene();
  } else if (ImGui::IsKeyPressed(ImGuiKey_O)) {
    open_scene();
  } else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
    if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
      save_scene_as();
    } else {
      save_scene();
    }
  } else if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
    if (command_history.can_undo()) {
      command_history.undo(graph);
      node_pos_refresh = true;
      on_graph_modified();
    }
  } else if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
    if (command_history.can_redo()) {
      command_history.redo(graph);
      node_pos_refresh = true;
      on_graph_modified();
    }
  } else if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
    quit();
  }
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

      if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
        quit();
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      ImGui::BeginDisabled(!command_history.can_undo());
      if (ImGui::MenuItem(ICON_FA_ARROW_ROTATE_LEFT "  Undo", "Ctrl+Z")) {
        command_history.undo(graph);
        node_pos_refresh = true;
      }
      ImGui::EndDisabled();

      ImGui::BeginDisabled(!command_history.can_redo());
      if (ImGui::MenuItem(ICON_FA_ARROW_ROTATE_RIGHT "  Redo", "Ctrl+Y")) {
        command_history.redo(graph);
        node_pos_refresh = true;
      }
      ImGui::EndDisabled();

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
      GraphFolder *folder = nullptr;
      if (!scene_->root_folders.empty()) {
        folder = scene_->root_folders[0].get();
      }

      if (folder) {
        [[maybe_unused]] auto *ref = add_graph_in_folder(*folder);
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
      // Upon popping up the rename popup, fill the buffer with the current name
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

static constexpr auto kAudioFileFilter =
  ".mod,.xm,.mp3";

void SceneEditorUI::render_timeline() {
  if (ImGui::Begin("Timeline")) {
    ImGui::BeginDisabled(current_graph_ref == nullptr);

    const bool flowing = is_time_flowing.load(std::memory_order_acquire);
    if (ImGui::Button(flowing ? ICON_FA_PAUSE " Pause" : ICON_FA_PLAY " Play")) {
      is_time_flowing.store(!flowing, std::memory_order_release);
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_SQUARE_PLUS " Add Segment")) {
      const std::string_view selected_graph_id = current_graph_ref->id;
      const TimelineSegment segment(0, 100, selected_graph_id);
      scene_->add_timeline_segment(segment);
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_FILE_AUDIO " Import Audio")) {
      open_audio_track_dialog();
    }
    ImGui::EndDisabled();

    if (selected_segment_ >= 0) {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, kDangerButton);
      if (ImGui::Button(ICON_FA_BAN " Remove Selected")) {
        scene_->remove_timeline_segment(static_cast<size_t>(selected_segment_));
        selected_segment_ = -1;
      }
      ImGui::PopStyleColor();
    }

    if (flowing) {
      current_frame_ += static_cast<int>(ImGui::GetIO().DeltaTime * scene_->fps);
      if (current_frame_ >= scene_->total_frames) {
        current_frame_ = 0;
      }
    }

    sync_audio_playback();

    ImGui::Separator();

    static bool expanded = true;
    ImSequencer::Sequencer(this,
                           &current_frame_, &expanded,
                           &selected_segment_,
                           &first_frame,
                           ImSequencer::SEQUENCER_EDIT_ALL);

    if (selected_segment_ >= 0 && selected_segment_ < static_cast<int>(scene_->timeline.size())) {
      const auto &seg = scene_->timeline[static_cast<size_t>(selected_segment_)];
      if (seg.type == SegmentType::GRAPH) {
        ImGui::Text("Graph: frames %d - %d, graph: %s", seg.frame_start, seg.frame_end,
                    seg.graph_id.empty() ? "(none)" : seg.graph_id.c_str());
      } else {
        const auto *track = scene_->get_audio_track(static_cast<size_t>(seg.audio_track_index));
        ImGui::Text("Audio: frames %d - %d, file: %s", seg.frame_start, seg.frame_end,
                    track ? track->title.c_str() : "(invalid)");
      }
    }
  }

  ImGui::End();
}

void SceneEditorUI::open_audio_track_dialog() {
  IGFD::FileDialogConfig cfg;
  cfg.path = ".";
  ImGuiFileDialog::Instance()->OpenDialog(
    kImportAudioDialogKey, "Import Audio Track", kAudioFileFilter, cfg);
}

void SceneEditorUI::sync_audio_playback() {
  const bool flowing = is_time_flowing.load(std::memory_order_acquire);
  const double current_time_s = static_cast<double>(current_frame_) / scene_->fps;

  for (const auto &seg: scene_->timeline) {
    if (seg.type != SegmentType::AUDIO || seg.audio_track_index < 0) continue;

    auto *track = scene_->get_audio_track(static_cast<size_t>(seg.audio_track_index));
    if (!track || !track->player) continue;

    const double seg_start_s = static_cast<double>(seg.frame_start) / scene_->fps;
    const double seg_end_s = static_cast<double>(seg.frame_end) / scene_->fps;
    const bool in_range = flowing && current_time_s >= seg_start_s && current_time_s < seg_end_s;

    if (in_range) {
      const double track_offset_s = current_time_s - seg_start_s + track->start_seconds;
      if (track->player->get_playback_state() != PlaybackState::PLAYING) {
        track->player->play();
      }
      track->player->seek(track_offset_s * 1000.0);
    } else {
      if (track->player->get_playback_state() == PlaybackState::PLAYING) {
        track->player->pause();
      }
    }
  }
}

void SceneEditorUI::render_top_status_bar() {
  // According to https://github.com/ocornut/imgui/issues/3518#issuecomment-918186716
  // this is the right way to implement a statusbar.
  constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_MenuBar;
  if (const float height = ImGui::GetFrameHeight(); ImGui::BeginViewportSideBar(
    "##TopStatusBar", nullptr, ImGuiDir_Up, height, window_flags)) {
    if (ImGui::BeginMenuBar()) {
      ImGui::TextUnformatted(scene_->name.c_str());

      if (scene_->dirty) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, .3f, .0f, 1.f), "*");
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Unsaved changes");
        }
      }

      ImGui::SameLine();
      ImGui::TextDisabled("%s", current_scene_path_.empty() ? "(unsaved)" : current_scene_path_.c_str());

      // Print the status message right after the scene title
      print_status_message();

      ImGui::EndMenuBar();
    }
    ImGui::End();
  }
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
    *type = static_cast<int>(segment.type);
  if (color)
    *color = segment.color;
}

const char *SceneEditorUI::GetItemLabel(const int index) const {
  if (index < 0 || index >= static_cast<int>(scene_->timeline.size())) {
    return "";
  }
  const auto &seg = scene_->timeline[static_cast<size_t>(index)];
  if (seg.type == SegmentType::GRAPH) {
    const auto *graph = scene_->find_graph(seg.graph_id);
    return graph ? graph->name.c_str() : "(unknown graph)";
  }
  const auto *track = scene_->get_audio_track(static_cast<size_t>(seg.audio_track_index));
  return track ? track->title.c_str() : "(unknown audio)";
}

void SceneEditorUI::Add(int type) {
  if (type == static_cast<int>(SegmentType::AUDIO)) {
    const TimelineSegment segment(0, 100, 0);
    scene_->add_timeline_segment(segment);
  } else {
    const TimelineSegment segment(0, 100, "");
    scene_->add_timeline_segment(segment);
  }
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
  current_frame_ = 0;
  selected_segment_ = -1;
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

  if (current_graph_ref && current_graph_ref->dirty) {
    save_current_graph();
  }

  if (scene_library_->save_scene(*scene_, current_scene_path_)) {
    scene_->dirty = false;
    set_status_message("Scene saved");
  }
}

void SceneEditorUI::save_scene_as() {
  IGFD::FileDialogConfig cfg;
  cfg.path = ".";
  ImGuiFileDialog::Instance()->OpenDialog(kSaveSceneDialogKey, "Save Scene As", ".nagscene", cfg);
}

void SceneEditorUI::save_current_graph() {
  if (current_graph_ref) {
    scene_->graph_data[current_graph_ref->id] = JsonGraphSerializer::serialize_graph(graph);
    current_graph_ref->dirty = false;
    set_status_message("Graph saved: " + current_graph_ref->name);
  }
}

void SceneEditorUI::load_graph_from_library(GraphReference &graph_ref) {
  current_graph_ref = &graph_ref;

  const auto it = scene_->graph_data.find(graph_ref.id);
  if (it == scene_->graph_data.end() || it->second.is_null()) {
    reset_graph();
    set_status_message("Empty graph: " + graph_ref.name);
    return;
  }

  NodeGraph loaded_graph;
  const auto result = JsonGraphSerializer::deserialize_graph(loaded_graph, it->second);
  if (!result) {
    spdlog::error("Failed to deserialize graph '{}': {}", graph_ref.name, result.error_message);
    set_status_message("Failed to load graph " + graph_ref.name);
    return;
  }

  command_history.clear();
  graph = std::move(loaded_graph);
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
    spdlog::warn("Graph '{}' has no output node", graph_ref.name);
  }

  set_status_message("Loaded: " + graph_ref.name);
}


GraphReference *SceneEditorUI::add_graph_in_folder(GraphFolder &folder,
                                                   const std::string_view graph_name) {
  auto *ref = scene_->add_graph(folder, std::string(graph_name));
  scene_->graph_data[ref->id] = nullptr;
  load_graph_from_library(*ref);
  return ref;
}
