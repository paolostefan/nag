#include "editor/graph_editor_ui.h"

#include <algorithm>
#include <unordered_set>

#include "IconsFontAwesome6.h"
#include "ImGuiFileDialog.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "editor/graph_commands.h"
#include "spdlog/spdlog.h"

#include "engine/node_registry.h"
#include "engine/nodes/particle_emitter_node.h"
#include "engine/shader_quad_helper.h"


GraphEditorUI::GraphEditorUI(
  std::string title, const int width, const int height) : UIWindow(std::move(title), width, height) {
  // Init graphics helpers
  ShaderQuadHelper::instance().initialize();

  // Initialize ImNodes
  ImNodes::CreateContext();
  editor_context = ImNodes::EditorContextCreate();

  // Configure ImNodes
  ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

  ImNodesIO &io = ImNodes::GetIO();
  io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
  io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

  ImNodesStyle &style = ImNodes::GetStyle();
  style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
}


void GraphEditorUI::main_event_loop() {
  bool running = true;
  SDL_Event event;

  // ReSharper disable once CppDFAConstantConditions
  while (running) {
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT) {
        running = false;
      }

      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE) {
        // Close main window → quit.
        if (event.window.windowID == SDL_GetWindowID(window)) {
          running = false;
        }

        // ★ Close preview window → just close the preview, keep running.
        if (preview_window.is_open() &&
            event.window.windowID == preview_window.GetWindowID()) {
          preview_window.close();
        }
      }
    }

    if (!running) {
      break;
    }

    // ── ImGui frame (main window) ──────────────────────────────────────────
    SDL_GL_MakeCurrent(window, gl_context);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    const ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(
      0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);

    render_ui();

    ImGui::Render();
    glViewport(0, 0,
               static_cast<int>(io->DisplaySize.x),
               static_cast<int>(io->DisplaySize.y));
    glClearColor(.1f, .1f, .1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);

    // Blit to preview window - only if open and output node is valid
    if (preview_window.is_open() &&
        !preview_window.is_paused() &&
        output_node != nullptr) {
      const Texture *tex = output_node->get_texture();
      preview_window.render(tex, gl_context, window);
    }
  }
}

// ============================================================================
// Graph Actions
// ============================================================================


void GraphEditorUI::save_graph(const std::string &path) {
  // ReSharper disable once CppUseStructuredBinding
  if (const auto result = serializer.save(graph, path)) {
    set_status_message(ICON_FA_CHECK "  Graph saved to " + path);
    spdlog::info("Graph saved to {}", path);
  } else {
    set_status_message(ICON_FA_TRIANGLE_EXCLAMATION "  Save failed: " + result.error_message);
    spdlog::error("Graph save failed: {}", result.error_message);
  }
}

// ============================================================================
// delete_selected_nodes
// ============================================================================

void GraphEditorUI::delete_selected_nodes() {
  // Get selected node IDs from ImNodes
  const int num_selected = ImNodes::NumSelectedNodes();
  if (num_selected == 0) return;

  std::vector<int> selected_nodes(num_selected);
  ImNodes::GetSelectedNodes(selected_nodes.data());

  // Convert to set for faster lookup
  std::unordered_set<int> nodes_to_delete;

  for (const int id: selected_nodes) {
    nodes_to_delete.insert(id);
  }

  delete_nodes(nodes_to_delete);
}

// ============================================================================
// delete_selected_links
// ============================================================================

void GraphEditorUI::delete_selected_links() {
  const int num_selected = ImNodes::NumSelectedLinks();
  if (num_selected == 0) return;

  std::vector<int> selected_links(num_selected);
  ImNodes::GetSelectedLinks(selected_links.data());

  std::unordered_set<int> links_to_delete;
  for (const int id: selected_links) {
    links_to_delete.insert(id);
  }

  delete_links(links_to_delete);
}

// ============================================================================
// render_ui
// ============================================================================

void GraphEditorUI::render_node_editor() {
  if (ImGui::Begin("Graph##GraphEditorWin", nullptr,
                   ImGuiWindowFlags_MenuBar
                   | ImGuiWindowFlags_NoDecoration
                   | ImGuiWindowFlags_NoCollapse
                   | ImGuiWindowFlags_NoResize
                   | ImGuiWindowFlags_NoMove)) {

    display_dialogs();

    ImNodes::EditorContextSet(editor_context);
    ImNodes::BeginNodeEditor();

    const bool refresh_positions = node_pos_refresh.exchange(false);
    const bool flowing = is_time_flowing.load(std::memory_order_acquire);

    // ── Nodes ──────────────────────────────────────────────────────────────────
    for (const auto &node: graph.nodes) {
      // ReSharper disable once CppDFAConstantConditions
      if (refresh_positions) {
        ImNodes::SetNodeScreenSpacePos(node->id, node->position);
      }

      ImNodes::BeginNode(node->id);

      ImNodes::BeginNodeTitleBar();
      if (auto *tn = dynamic_cast<TimeNode *>(node.get())) {
        ImGui::Text("Time [%.2f s]", tn->time);

        if (flowing) {
          tn->step(ImGui::GetIO().DeltaTime);
          graph.evaluate();
        }
      } else if (auto *pe = dynamic_cast<ParticleEmitterNode *>(node.get())) {
        ImGui::Text("%s [%zu]", node->name.c_str(), pe->particle_count());

        if (flowing) {
          pe->step(ImGui::GetIO().DeltaTime);
          graph.evaluate();
        }
      } else {
        ImGui::TextUnformatted(node->name.c_str());
      }

      ImNodes::EndNodeTitleBar();

      if (node->inputs.size() + node->outputs.size() == 0) {
        ImGui::TextUnformatted("No in/out?!");
      }

      for (const auto &pin: node->inputs) {
        ImNodes::PushColorStyle(ImNodesCol_Pin, get_pin_color(pin));
        ImNodes::BeginInputAttribute(pin.id);
        ImGui::TextUnformatted(pin.name.c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
      }

      if (const auto *visual_node = dynamic_cast<VisualNode *>(node.get())) {
        render_visual_node_body(visual_node);
      }

      for (const auto &pin: node->outputs) {
        ImNodes::PushColorStyle(ImNodesCol_Pin, get_pin_color(pin));
        ImNodes::BeginOutputAttribute(pin.id);
        ImGui::TextUnformatted(pin.name.c_str());
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
      }

      ImNodes::EndNode();
    }

    node_pos_refresh = false;

    // ── Links ──────────────────────────────────────────────────────────────────
    for (const auto &[id, start_pin_id, end_pin_id]: graph.links) {
      ImNodes::Link(id, start_pin_id, end_pin_id);
    }

    ImNodes::EndNodeEditor();

    // Sync ImNodes positions back into node data every frame
    for (const auto &node: graph.nodes) {
      node->position = ImNodes::GetNodeScreenSpacePos(node->id);
    }

    // ── Detect Right Click on Canvas ───────────────────────────────────────────
    // Must come after EndNodeEditor() and before OpenPopup()

    static int hovered_node_id{0}, hovered_link_id{0};

    const bool should_open_add_menu =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        !ImNodes::IsNodeHovered(&hovered_node_id) &&
        !ImNodes::IsLinkHovered(&hovered_link_id);
    if (should_open_add_menu) {
      ImGui::OpenPopup("add_node_popup");
    }

    // ── Node/Link Context Menu (right click on selected) ───────────────────────
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      if (ImNodes::NumSelectedNodes() > 0 ||
          ImNodes::NumSelectedLinks() > 0) {
        ImGui::OpenPopup("delete_selection_popup");
      }
    }

    // ── Context Menu ───────────────────────────────────────────────────────────
    render_context_menu();

    // ── Link Creation ──────────────────────────────────────────────────────────
    int start_pin_id, end_pin_id;
    if (ImNodes::IsLinkCreated(&start_pin_id,
                               &end_pin_id)) {
      auto add_link_command = std::make_unique<AddLinkCommand>(
        start_pin_id, end_pin_id);

      command_history.execute(graph, std::move(add_link_command));
    }

    // ── Link Deletion ──────────────────────────────────────────────────────────
    int link_id;
    if (ImNodes::IsLinkDestroyed(&link_id)) {
      auto delete_links_command = std::make_unique<DeleteLinksCommand>(link_id);

      command_history.execute(graph, std::move(delete_links_command));
    }

    // ==========================================================================
    // ---- Keyboard Shortcuts --------------------------------------------------
    // ==========================================================================

    // Delete selected nodes/links with Delete or Backspace key
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
      if (ImGui::IsKeyPressed(ImGuiKey_Delete) ||
          ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        const auto node_no = ImNodes::NumSelectedNodes();
        const auto link_no = ImNodes::NumSelectedLinks();

        // Try deleting nodes first (higher priority)
        if (node_no > 0) {
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
    }
  }
  ImGui::End(); // Node editor
}

void GraphEditorUI::render_ui() {

  render_menu_bar();

  render_node_editor();

  render_node_props();
}

void GraphEditorUI::render_menu_bar() {
  if (!ImGui::BeginMainMenuBar()) return;

  // ── File Menu ──────────────────────────────────────────────────────────────
  if (ImGui::BeginMenu(ICON_FA_FILE "  File")) {
    if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save Graph", "Ctrl+S")) {
      IGFD::FileDialogConfig cfg;
      cfg.path = ".";
      cfg.fileName = "graph.json";
      cfg.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
      ImGuiFileDialog::Instance()->OpenDialog(
        kSaveGraphDialogKey, "Save Graph", kFileFilter, cfg
      );
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Load Graph", "Ctrl+O")) {
      IGFD::FileDialogConfig cfg;
      cfg.path = ".";
      ImGuiFileDialog::Instance()->OpenDialog(
        kLoadGraphDialogKey, "Load Graph", kFileFilter, cfg
      );
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ICON_FA_ROTATE_LEFT "  Reset to Default")) {
      reset_graph();
      build_default_graph();
      set_status_message(ICON_FA_CHECK "  Graph reset to default");
    }

    ImGui::EndMenu();
  }

  // ── Edit Menu ──────────────────────────────────────────────────────────────
  if (ImGui::BeginMenu("Edit")) {
    if (ImGui::MenuItem("Undo",
                        "Ctrl+Z",
                        false,
                        command_history.can_undo())) {
      command_history.undo(graph);
      node_pos_refresh.store(true);
    }

    if (ImGui::MenuItem("Redo",
                        "Ctrl+Shift+Z",
                        false,
                        command_history.can_redo())) {
      command_history.redo(graph);
      node_pos_refresh.store(true);
    }

    ImGui::EndMenu();
  }

  // ── View menu ─────────────────────

  if (ImGui::BeginMenu(ICON_FA_TV "  View")) {
    // Toggle preview window.
    if (ImGui::MenuItem(ICON_FA_DISPLAY " Preview Window",
                        nullptr,
                        preview_window.is_open())) {
      if (preview_window.is_open()) {
        preview_window.close();
      } else {
        preview_window.open(window, gl_context);
      }
    }

    const bool flowing = is_time_flowing.load(std::memory_order_acquire);
    if (ImGui::MenuItem(ICON_FA_PLAY " Live",
                        nullptr,
                        flowing)) {
      is_time_flowing.store(!flowing, std::memory_order_release);
    }

    ImGui::EndMenu();
  }

  print_status_message();

  ImGui::EndMainMenuBar();
}

void GraphEditorUI::display_dialogs() {
  // ── File Dialog: Save ──────────────────────────────────────────────────────
  constexpr ImVec2 kDialogSize{600.f, 400.f};
  if (ImGuiFileDialog::Instance()->Display(
    kSaveGraphDialogKey, ImGuiWindowFlags_NoCollapse, kDialogSize)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      save_graph(ImGuiFileDialog::Instance()->GetFilePathName());
    }
    ImGuiFileDialog::Instance()->Close();
  }

  // ── File Dialog: Load ──────────────────────────────────────────────────────
  if (ImGuiFileDialog::Instance()->Display(
    kLoadGraphDialogKey, ImGuiWindowFlags_NoCollapse, kDialogSize)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const auto path = ImGuiFileDialog::Instance()->GetFilePathName();
      if (auto result = load_graph(path); !result) {
        set_status_message(ICON_FA_TRIANGLE_EXCLAMATION "  Load failed: " +
                           result.error_message);
      } else {
        set_status_message(ICON_FA_CHECK "  Graph loaded from " + path);
        spdlog::info("Graph loaded from {}", path);
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }
}

// ============================================================================
// render_context_menu
// ============================================================================

void GraphEditorUI::render_context_menu() {
  if (ImGui::BeginPopup("add_node_popup")) {
    // Screen space
    const ImVec2 spawn_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

    ImGui::TextDisabled(ICON_FA_PLUS "  Add Node");
    ImGui::Separator();

    // Get registry categories
    const auto categories = NodeRegistry::instance().get_nodes_by_category();

    // @todo use just a single array of structs for both order and icons, to avoid parallel arrays and string comparisons
    // View order
    static constexpr const char *const kCategoryOrder[] = {
      "Visual", "Effects", "Math", "Temporal", "Generators"
    };

    static constexpr const char *const kCategoryIcons[] = {
      ICON_FA_PAINTBRUSH "  ",
      ICON_FA_WAND_MAGIC_SPARKLES "  ",
      ICON_FA_CALCULATOR "  ",
      ICON_FA_CLOCK "  ",
      ICON_FA_BOLT "  ",
    };

    // Category icons

    // Show categories in the predefined order (and additional ones, if any, afterward)
    auto render_category = [&](const std::string &category) {
      const auto it = categories.find(category);
      if (it == categories.end()) return;

      auto cat_icon = ICON_FA_CIRCLE_NODES "  ";
      for (int i = 0; i < std::size(kCategoryOrder); ++i) {
        if (kCategoryOrder[i] == category) {
          cat_icon = kCategoryIcons[i];
          break;
        }
      }

      const std::string label =
          std::string(cat_icon) + category;

      if (!ImGui::BeginMenu(label.c_str())) return;

      for (const NodeType type: it->second) {
        const auto *info = NodeRegistry::instance().get_type_info(type);
        if (!info) continue;

        // Menu item Tooltip
        if (ImGui::MenuItem(info->display_name.c_str())) {
          spawn_node(type, spawn_pos);
        }

        if (ImGui::IsItemHovered() && !info->description.empty()) {
          ImGui::SetTooltip("%s", info->description.c_str());
        }
      }

      ImGui::EndMenu();
    };

    for (const auto *cat: kCategoryOrder) {
      render_category(cat);
    }

    // Categories not in kCategoryOrder
    for (const auto &cat: categories | std::views::keys) {
      const bool already_shown = std::ranges::any_of(
        kCategoryOrder,
        [&cat](const char *c) { return cat == c; }
      );
      if (!already_shown) render_category(cat);
    }

    ImGui::EndPopup();
  } // add node popup

  if (ImGui::BeginPopup("delete_selection_popup")) {
    const int num_nodes = ImNodes::NumSelectedNodes();
    const int num_links = ImNodes::NumSelectedLinks();

    if (num_nodes > 0) {
      const std::string label = num_nodes == 1
                                  ? ICON_FA_TRASH "  Delete Node"
                                  : ICON_FA_TRASH "  Delete " + std::to_string(num_nodes) + " Nodes";

      if (ImGui::MenuItem(label.c_str(), "Del")) {
        delete_selected_nodes();
        set_status_message("Deleted " + std::to_string(num_nodes) + (num_nodes == 1 ? " node" : " nodes"));
      }
    }

    if (num_links > 0) {
      const std::string label = num_links == 1
                                  ? ICON_FA_LINK_SLASH "  Delete Link"
                                  : ICON_FA_LINK_SLASH "  Delete " + std::to_string(num_links) + " Links";

      if (ImGui::MenuItem(label.c_str(), "Del")) {
        delete_selected_links();
        set_status_message("Deleted " + std::to_string(num_links) + (num_links == 1 ? " link" : " links"));
      }
    }

    ImGui::EndPopup();
  } // delete selection popup
}

// ============================================================================
// Helpers
// ============================================================================

void GraphEditorUI::render_visual_node_body(
  const VisualNode *visual_node) {
  if (!visual_node ||
      !visual_node->render_target ||
      !visual_node->render_target->is_valid()) {
    ImGui::TextDisabled("(no texture)");
    return;
  }

  constexpr float kPreviewWidth = 150.f;
  const float aspect =
      static_cast<float>(visual_node->render_target->get_height()) /
      static_cast<float>(visual_node->render_target->get_width());

  ImGui::Image(
    visual_node->render_target->get_texture(),
    ImVec2(kPreviewWidth, kPreviewWidth * aspect),
    ImVec2(0, 1), ImVec2(1, 0)
  );
}

unsigned int GraphEditorUI::get_pin_color(const Pin &pin) {
  if (*pin.data_type == typeid(float)) return ImColor(100, 200, 100);
  if (*pin.data_type == typeid(Texture *)) return ImColor(200, 100, 200);
  if (*pin.data_type == typeid(void)) return ImColor(150, 150, 150);
  return ImColor(100, 100, 200);
}
