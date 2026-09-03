#include "editor/scene_editor_ui.h"

#include <algorithm>
#include <fstream>

#include <IconsFontAwesome6.h>
#include <imgui_internal.h>

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "imnodes.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "editor/audio_playback_controller.h"

namespace {
  /// @brief Extract audio segments from the scene timeline.
  std::vector<audio_playback::AudioSegment> to_audio_segments(const Scene &scene) {
    std::vector<audio_playback::AudioSegment> segments;
    for (const auto &seg: scene.timeline) {
      if (seg.type != SegmentType::AUDIO) continue;
      segments.push_back({
        seg.audio_track_index,
        seg.frame_start,
        seg.frame_end
      });
    }
    return segments;
  }
} // namespace

SceneEditorUI::SceneEditorUI()
  : GraphEditorUI("Scene Editor", 1280, 800),
    recent_files_(get_recent_scenes_path()) {

  scene_library_ = std::make_unique<SceneLibrary>();

  // Create a new empty scene (also wires scene-dependent modules)
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
    graph_scene_sync_->sync_graph(*current_graph_ref, graph);
  }

  recent_files_.load();
}

void SceneEditorUI::wire_scene_dependencies() {
  timeline_controller_ = std::make_unique<TimelineController>(*scene_);
  graph_scene_sync_ = std::make_unique<GraphSceneSync>(*scene_);

  GraphLibraryPanel::Callbacks cb;
  cb.on_open_graph = [this](GraphReference &ref) { load_graph_from_library(ref); };
  cb.on_add_graph = [this](GraphFolder &folder) -> GraphReference * {
    return add_graph_in_folder(folder);
  };
  cb.on_add_folder = [this]() { [[maybe_unused]] auto *folder = scene_->add_folder(nullptr, "New Folder"); };
  cb.on_graph_deleted = [this](const std::string &graph_id) {
    graph_scene_sync_->remove_graph_data(graph_id);
    select_first_available_graph();
  };

  graph_library_panel_ = std::make_unique<GraphLibraryPanel>(
    *scene_, current_graph_ref, std::move(cb));
}

void SceneEditorUI::render_ui() {
  handle_keyboard_shortcuts();

  render_menu_bar();

  render_top_status_bar();

  render_node_editor();

  render_node_props();

  graph_library_panel_->render();
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
        wire_scene_dependencies();
        current_scene_path_ = scene_path;
        reset_graph();
        set_status_message("Scene loaded: " + scene_->name);
        recent_files_.add(scene_path);
        recent_files_.save();

        // Auto-select the first graph in the library if it exists
        if (auto *first = graph_scene_sync_->find_first_graph()) {
          load_graph_from_library(*first);
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
  if (auto *first = graph_scene_sync_->find_first_graph()) {
    load_graph_from_library(*first);
  } else {
    current_graph_ref = nullptr;
    reset_graph();
  }
}

void SceneEditorUI::on_graph_modified() {
  if (current_graph_ref) {
    graph_scene_sync_->sync_graph(*current_graph_ref, graph);
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
      if (ImGui::BeginMenu("Recent Scenes")) {
        if (recent_files_.get().empty()) {
          ImGui::BeginDisabled();
          ImGui::MenuItem("(no recent scenes)", nullptr, false, false);
          ImGui::EndDisabled();
        } else {
          auto it = recent_files_.get().begin();
          while (it != recent_files_.get().end()) {
            if (ImGui::MenuItem(it->c_str())) {
              const std::string path = *it;
              auto loaded = SceneLibrary::load_scene(path);
              if (loaded) {
                scene_ = std::move(loaded);
                wire_scene_dependencies();
                current_scene_path_ = path;
                reset_graph();
                set_status_message("Scene loaded: " + scene_->name);
                recent_files_.add(path);
                recent_files_.save();
                if (auto *first = graph_scene_sync_->find_first_graph()) {
                  load_graph_from_library(*first);
                }
              } else {
                spdlog::warn("Recent scene not found, removing: {}", path);
                recent_files_.remove(path);
                recent_files_.save();
                set_status_message("Recent scene not found");
              }
              break;
            }
            ++it;
          }
        }
        ImGui::EndMenu();
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

    audio_playback::sync(
      scene_->audio_tracks,
      to_audio_segments(*scene_),
      scene_->fps,
      current_frame_,
      flowing
    );

    ImGui::Separator();

    static bool expanded = true;
    ImSequencer::Sequencer(timeline_controller_.get(),
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

void SceneEditorUI::new_scene() {
  scene_ = std::make_unique<Scene>("New Scene");
  wire_scene_dependencies();
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

  if (current_graph_ref) {
    save_current_graph();
  }

  if (scene_library_->save_scene(*scene_, current_scene_path_)) {
    scene_->dirty = false;
    set_status_message("Scene saved");
    recent_files_.add(current_scene_path_);
    recent_files_.save();
  }
}

void SceneEditorUI::save_scene_as() {
  IGFD::FileDialogConfig cfg;
  cfg.path = ".";
  ImGuiFileDialog::Instance()->OpenDialog(kSaveSceneDialogKey, "Save Scene As", ".nagscene", cfg);
}

void SceneEditorUI::save_current_graph() {
  if (current_graph_ref) {
    graph_scene_sync_->sync_graph(*current_graph_ref, graph);
    set_status_message("Graph saved: " + current_graph_ref->name);
  }
}

void SceneEditorUI::load_graph_from_library(GraphReference &graph_ref) {
  current_graph_ref = &graph_ref;

  auto loaded_graph = graph_scene_sync_->load_graph(graph_ref);
  if (!loaded_graph) {
    reset_graph();
    set_status_message("Empty graph: " + graph_ref.name);
    return;
  }

  command_history.clear();
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
    spdlog::warn("Graph '{}' has no output node", graph_ref.name);
  }

  set_status_message("Loaded: " + graph_ref.name);
}


GraphReference *SceneEditorUI::add_graph_in_folder(GraphFolder &folder,
                                                   const std::string_view graph_name) {
  auto *ref = graph_scene_sync_->add_graph(folder, graph_name);
  if (ref) {
    load_graph_from_library(*ref);
  }
  return ref;
}

// ── Recent Scenes ─────────────────────────────────────────────────────────────

std::filesystem::path SceneEditorUI::get_recent_scenes_path() {
  const char *xdg_config = std::getenv("XDG_CONFIG_HOME");
  std::filesystem::path dir;
  if (xdg_config && *xdg_config) {
    dir = std::filesystem::path(xdg_config) / "nag";
  } else {
    const char *home = std::getenv("HOME");
    dir = std::filesystem::path(home ? home : ".") / ".config" / "nag";
  }
  std::filesystem::create_directories(dir);
  return dir / kRecentScenesFile;
}
