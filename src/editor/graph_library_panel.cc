#include "editor/graph_library_panel.h"

#include <cstring>
#include <memory>

#include <IconsFontAwesome6.h>

#include "imgui.h"

#include "editor/command_history.h"
#include "editor/scene_commands.h"

GraphLibraryPanel::GraphLibraryPanel(Scene &scene, GraphReference *&current_graph,
                                     Callbacks callbacks, CommandHistory *history)
  : scene_(scene), current_graph_(current_graph), callbacks_(std::move(callbacks)), history_(history) {
}

void GraphLibraryPanel::render() {
  if (ImGui::Begin("Graph Library")) {
    if (ImGui::Button(ICON_FA_FOLDER_PLUS "  New folder")) {
      if (callbacks_.on_add_folder) {
        callbacks_.on_add_folder();
      } else if (history_) {
        history_->execute(scene_, std::make_unique<AddFolderCommand>("", "New Folder"));
      } else {
        [[maybe_unused]] auto *folder = scene_.add_folder(nullptr, "New Folder");
      }
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_DIAGRAM_PROJECT "  New graph")) {
      GraphFolder *folder = nullptr;
      if (!scene_.root_folders.empty()) {
        folder = scene_.root_folders[0].get();
      }

      if (folder && callbacks_.on_add_graph) {
        callbacks_.on_add_graph(*folder);
      }
    }

    ImGui::Separator();

    render_folder_tree(scene_.root_folders);
  }

  ImGui::End();

  render_rename_popup();

  // ── Confirm Delete Graph ──────────────────────────────────────────────────
  if (!pending_delete_graph_id_.empty()) {
    ImGui::OpenPopup("Delete Graph##Modal");
    if (ImGui::BeginPopupModal("Delete Graph##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      const auto *graph = scene_.find_graph(pending_delete_graph_id_);
      ImGui::Text("Delete graph \"%s\"?", graph ? graph->name.c_str() : "(unknown)");
      ImGui::Separator();

      if (ImGui::Button("Delete", ImVec2(100, 0))) {
        const std::string deleted_id = pending_delete_graph_id_;
        if (history_) {
          history_->execute(scene_, std::make_unique<RemoveGraphCommand>(deleted_id));
        } else {
          scene_.remove_graph(deleted_id);
        }
        pending_delete_graph_id_.clear();
        if (callbacks_.on_graph_deleted) {
          callbacks_.on_graph_deleted(deleted_id);
        }
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
}

void GraphLibraryPanel::render_folder_tree(
  const std::vector<std::unique_ptr<GraphFolder> > &folders) {
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
        if (history_) {
          history_->execute(scene_, std::make_unique<AddFolderCommand>(folder->id, "New Subfolder"));
        } else {
          [[maybe_unused]] auto *subfolder = scene_.add_folder(folder->id, "New Subfolder");
        }
      }
      if (ImGui::MenuItem(ICON_FA_DIAGRAM_PROJECT "  Add Graph Here")) {
        if (callbacks_.on_add_graph) {
          callbacks_.on_add_graph(*folder);
        }
      }

      if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename")) {
        rename_target_ = RenameTargetFolder;
        rename_target_id_ = folder->id;
        is_renaming_.store(true, std::memory_order_release);
      }

      ImGui::Separator();

      if (ImGui::MenuItem(ICON_FA_TRASH "  Delete Folder")) {
        if (history_) {
          history_->execute(scene_, std::make_unique<RemoveFolderCommand>(folder->id));
        } else {
          scene_.remove_folder(folder->id);
        }
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

        const bool is_selected = &graph_ref == current_graph_;
        if (is_selected) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        }

        if (ImGui::TreeNodeEx(graph_ref.id.c_str(), kGraphFlags,
                              "%s%s", graph_ref.name.c_str(), graph_ref.dirty ? " *" : "")) {
          if (ImGui::IsItemClicked() && &graph_ref != current_graph_) {
            if (callbacks_.on_open_graph) {
              callbacks_.on_open_graph(graph_ref);
            }
          }

          if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename")) {
              rename_target_ = RenameTargetGraph;
              rename_target_id_ = graph_ref.id;
              is_renaming_.store(true, std::memory_order_release);
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_TRASH "  Delete")) {
              pending_delete_graph_id_ = graph_ref.id;
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
    }
  }
}

void GraphLibraryPanel::render_rename_popup() {
  if (is_renaming_.load(std::memory_order_acquire)) {
    ImGui::OpenPopup("RenamePopup");
  }

  if (ImGui::BeginPopup("RenamePopup")) {
    static char name_buffer[256];

    if (ImGui::IsWindowAppearing()) {
      ImGui::SetKeyboardFocusHere();
      switch (rename_target_) {
        case RenameTargetScene:
          strncpy(name_buffer, scene_.name.c_str(), sizeof(name_buffer));
          break;
        case RenameTargetFolder: {
          if (const auto *folder = scene_.find_folder(rename_target_id_)) {
            strncpy(name_buffer, folder->name.c_str(), sizeof(name_buffer));
          }
          break;
        }
        case RenameTargetGraph: {
          if (const auto *graph = scene_.find_graph(rename_target_id_)) {
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
          scene_.name = new_name;
          break;
        case RenameTargetFolder:
          if (history_) {
            history_->execute(scene_, std::make_unique<RenameFolderCommand>(rename_target_id_, new_name));
          } else {
            scene_.rename_folder(rename_target_id_, new_name);
          }
          break;
        case RenameTargetGraph:
          if (history_) {
            history_->execute(scene_, std::make_unique<RenameGraphCommand>(rename_target_id_, new_name));
          } else {
            scene_.rename_graph(rename_target_id_, new_name);
          }
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
