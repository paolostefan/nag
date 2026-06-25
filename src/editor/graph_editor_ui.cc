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
#include "engine/shader_quad_helper.h"
#include "engine/nodes/particle_emitter_node.h"
#include "engine/nodes/particle_system_node.h"


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
  SDL_Event event;

  while (running.load(std::memory_order_acquire)) {
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT) {
        running.store(false, std::memory_order_release);
      }

      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE) {
        // Close main window → quit.
        if (event.window.windowID == SDL_GetWindowID(window)) {
          running.store(false, std::memory_order_release);
        }

        // ★ Close preview window → just close the preview, keep running.
        if (preview_window.is_open() &&
            event.window.windowID == preview_window.GetWindowID()) {
          preview_window.close();
        }
      }
    }

    if (!running.load(std::memory_order_acquire)) {
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

// ----------------------------------------------------------------------------
// delete_selected_nodes
// ----------------------------------------------------------------------------

void GraphEditorUI::delete_selected_nodes() {
  // Get selected node IDs from ImNodes
  const int num_selected = ImNodes::NumSelectedNodes();
  if (num_selected == 0) return;

  int selected_nodes[num_selected + 1];
  ImNodes::GetSelectedNodes(selected_nodes);

  // Convert to set to avoid errors
  const std::unordered_set nodes_to_delete(selected_nodes, selected_nodes + num_selected);
  delete_nodes(nodes_to_delete);

  ImNodes::ClearNodeSelection();
}

// ----------------------------------------------------------------------------
// duplicate_selected_nodes
// ----------------------------------------------------------------------------

void GraphEditorUI::duplicate_selected_nodes() {
  // Get selected node IDs from ImNodes
  const int num_selected = ImNodes::NumSelectedNodes();
  if (num_selected == 0) return;

  int selected_nodes[num_selected + 1];
  ImNodes::GetSelectedNodes(selected_nodes);

  // Convert to set to avoid errors
  const std::unordered_set nodes_to_copy(selected_nodes, selected_nodes + num_selected);

  // Offset position

  // Create duplicated nodes
  for (const int old_id: nodes_to_copy) {
    constexpr float kOffset = 50.f;
    Node *old_node = graph.find_node(old_id);
    if (!old_node) continue;

    auto new_node = NodeRegistry::instance().create_node(old_node->type);
    if (!new_node) continue;

    [[maybe_unused]] auto res = new_node->deserialize_params(old_node->serialize_params());
    new_node->name = old_node->name;
    new_node->position.x = old_node->position.x + kOffset;
    new_node->position.y = old_node->position.y + kOffset;

    if (auto *vis = dynamic_cast<VisualNode *>(new_node.get())) {
      if (auto *old_vis = dynamic_cast<VisualNode *>(old_node)) {
        if (old_vis->render_target &&
            old_vis->render_target->is_valid()) {
          vis->initialize(
            old_vis->render_target->get_width(),
            old_vis->render_target->get_height()
          );
        }
      }
    } // if visual node

    Node *added_node = graph.add_node(std::move(new_node));
    // Todo keep a map of old->new to recreate links
  }

  ImNodes::ClearNodeSelection();
  node_pos_refresh.store(true, std::memory_order_release);
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

      // Render title bar
      ImNodes::BeginNodeTitleBar();

      ImGui::TextUnformatted(node->name.c_str());

      switch (node->type) {
        case NodeType::Time: {
          if (flowing) {
            ((TimeNode *) node.get())->step(ImGui::GetIO().DeltaTime);
            graph.evaluate();
          }
        }
        break;
        default:
          break;
      } // switch node type

      ImNodes::EndNodeTitleBar();

      switch (node->type) {
        case NodeType::Time:
          ImGui::TextDisabled("%.02f s", ((TimeNode *) node.get())->time);
          break;

        case NodeType::Output: {
          const auto *out_node = (OutputNode *) node.get();

          const auto *tx = out_node->get_texture();
          if (!tx) {
            ImGui::TextColored(kWarningTextCol, "No output texture");
            break;
          }

          constexpr float kPreviewWidth = 150.f;
          const float aspect =
              static_cast<float>(tx->height) /
              static_cast<float>(tx->width);

          ImGui::Image(tx->texture_id,
            ImVec2(kPreviewWidth, kPreviewWidth * aspect),
            ImVec2(0, 1), ImVec2(1, 0)
          );
        }
        break;

        case NodeType::Abs:
        case NodeType::Add:
        case NodeType::Constant:
        case NodeType::Cos:
        case NodeType::Divide:
        case NodeType::LFO:
        case NodeType::Lerp:
        case NodeType::Modulo:
        case NodeType::Negate:
        case NodeType::Multiply:
        case NodeType::Random:
        case NodeType::Sin:
        case NodeType::Smoother:
        case NodeType::SmoothStep:
        case NodeType::Subtract:
        case NodeType::Tan:

          // Show output value in smaller font, muted color
          // Show output value in math nodes
          ImGui::TextDisabled("%0.03f", *node->outputs[0].get_float());
          break;

        case NodeType::ParticleEmitter: {
          ImGui::TextDisabled(
            "%0.01f p/s", node->get_param("rate"));
        }
        break;

        case NodeType::ParticleSystem: {
          const auto *particle_system_node = (ParticleSystemNode *) node.get();
          ImGui::TextDisabled("%d particles", particle_system_node->population);
          ImGui::TextDisabled("%d died", particle_system_node->casualties);
        }
        break;

        default:
          break;
      } // switch node type


      // Render input/output pins
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
      ImGui::OpenPopup(kAddNodePopup);
    }

    // ── Node/Link Context Menu (right click on selected) ───────────────────────
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      if (ImNodes::NumSelectedNodes() > 0 ||
          ImNodes::NumSelectedLinks() > 0) {
        ImGui::OpenPopup(kSelNodePopup);
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
  if (ImGui::BeginPopup(kAddNodePopup)) {
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

      auto sorted_types = it->second;
      std::ranges::sort(sorted_types, [](NodeType a, NodeType b) {
        const auto *ai = NodeRegistry::instance().get_type_info(a);
        const auto *bi = NodeRegistry::instance().get_type_info(b);
        return ai && bi && ai->display_name < bi->display_name;
      });

      for (const NodeType type: sorted_types) {
        const auto *info = NodeRegistry::instance().get_type_info(type);
        if (!info) continue;

        // Menu item Tooltip
        if (ImGui::MenuItem(info->display_name.c_str())) {
          spawn_node(type, spawn_pos);
          ImNodes::ClearNodeSelection();
          node_pos_refresh.store(true, std::memory_order_release);
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

  if (ImGui::BeginPopup(kSelNodePopup)) {
    const int num_nodes = ImNodes::NumSelectedNodes();
    const int num_links = ImNodes::NumSelectedLinks();

    if (num_nodes > 0) {
      std::string label = num_nodes == 1
                            ? ICON_FA_COPY "  Duplicate Node"
                            : ICON_FA_COPY "  Duplicate " + std::to_string(num_nodes) + " Nodes";

      if (ImGui::MenuItem(label.c_str(), "Ctrl+D")) {
        duplicate_selected_nodes();
        set_status_message(
          ICON_FA_COPY " Duplicated " + std::to_string(num_nodes) + (num_nodes == 1 ? " node" : " nodes"));
      }

      label = num_nodes == 1
                ? ICON_FA_TRASH "  Delete Node"
                : ICON_FA_TRASH "  Delete " + std::to_string(num_nodes) + " Nodes";

      if (ImGui::MenuItem(label.c_str(), "Del")) {
        delete_selected_nodes();
        set_status_message(
          ICON_FA_TRASH " Deleted " + std::to_string(num_nodes) + (num_nodes == 1 ? " node" : " nodes"));
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

void GraphEditorUI::render_visual_node_body(const VisualNode *visual_node) {
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
  if (pin.data_type == DataType::Particles2D) return ImColor(200, 200, 100);
  if (pin.data_type == DataType::Float) return ImColor(100, 200, 100);
  if (pin.data_type == DataType::Texture) return ImColor(200, 100, 200);
  return ImColor(100, 100, 200);
}
