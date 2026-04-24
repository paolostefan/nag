#include "graph_visual_insight.h"

#include <algorithm>
#include <unordered_set>

#include "IconsFontAwesome6.h"
#include "ImGuiFileDialog.h"
#include "implot.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "editor/graph_commands.h"
#include "spdlog/spdlog.h"

#include "editor/scrolling_buffer.h"
#include "engine/nodes/math_nodes.h"
#include "engine/node_registry.h"
#include "engine/shader_quad_helper.h"
#include "engine/nodes/clear_color_node.h"
#include "engine/nodes/composite_node.h"
#include "engine/nodes/gradient_node.h"
#include "engine/nodes/temporal_nodes.h"


GraphVisualInsight::GraphVisualInsight() : UIWindow("Graph insight POC",
                                                    1024, 768),
                                           GraphEditor() {
  // Init stuff
  ShaderQuadHelper::instance().initialize();
  GraphVisualInsight::build_default_graph();

  // ===============

  ImNodes::CreateContext();
  editor_context = ImNodes::EditorContextCreate();

  // from example imnodes code
  ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

  ImNodesIO &io = ImNodes::GetIO();
  io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
  io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

  ImNodesStyle &style = ImNodes::GetStyle();
  style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
}


void GraphVisualInsight::main_event_loop() {
  bool running = true;
  SDL_Event event;

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

    if (!running) break;

    // ── ImGui frame (main window) ──────────────────────────────────────────
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

    // Blit to preview window - only if open, not paused, and output node is valid
    if (preview_window.is_open() &&
        !preview_window.is_paused() &&
        output_node != nullptr) {
      const Texture *tex = output_node->get_texture();
      preview_window.render(tex, gl_context, window);
    }
  }
}

// hardcoded default graph
void GraphVisualInsight::build_default_graph() {
  if (time_node == nullptr || output_node == nullptr) {
    spdlog::error("Default graph requires TimeNode and OutputNode");
    return;
  }

  // Noise node
  auto noise_node = NoiseNode::create(1.f, 4.f, 3, 0.8f);
  noise_node->position = {150, 30};
  const auto noise_ptr = graph.add_node(std::move(noise_node));
  graph.add_link(time_node->outputs[0], noise_ptr->inputs[0]);

  // Oscillator A
  auto lfo_a = LFONode::create(1.3f, 2.5f, LFONode::WaveShape::Sawtooth);
  lfo_a->name = "Oscillator A";
  lfo_a->position = {150, 110};
  const Node *sin_a = graph.add_node(std::move(lfo_a));
  graph.add_link(time_node->outputs[0], sin_a->inputs[0]);

  // Oscillator B node connected to time
  auto lfo_b_node = LFONode::create(.7f, 1.f, LFONode::WaveShape::Sine, 0, .5f);
  lfo_b_node->name = "Oscillator B";
  lfo_b_node->position = {150, 190};
  const Node *lfo_b = graph.add_node(std::move(lfo_b_node));
  graph.add_link(time_node->outputs[0], lfo_b->inputs[0]);

  // Adding node connected to A, B and Noise
  auto adding_node = AddNode::create(3);
  adding_node->position = {290, 30};
  const auto adding = graph.add_node(std::move(adding_node));
  graph.add_link(noise_ptr->outputs[0], adding->inputs[0]);
  graph.add_link(sin_a->outputs[0], adding->inputs[1]);
  graph.add_link(lfo_b->outputs[0], adding->inputs[2]);

  // Visual: ClearColor
  auto color_node = ClearColorNode::create(Color::yellow());
  if (!color_node->initialize(400, 300)) {
    throw std::runtime_error("ClearColorNode: failed to initialize");
  }
  color_node->position = {420, 220};
  const auto color = graph.add_node(std::move(color_node));
  graph.add_link(lfo_b->outputs[0], color->inputs[1] /* The green component will oscillate */);

  // Visual: GradientNode (dangling)
  auto grad_node = GradientNode::create(
    GradientNode::Type::Radial, Color::cyan(), Color::transparent()
  );
  grad_node->position = {420, 40};
  grad_node->initialize(400, 300);
  grad_node->evaluate(); // TODO: remove this, it's here only because the node is dangling
  const auto grad = graph.add_node(std::move(grad_node));

  auto composite_node = CompositeNode::create(CompositeNode::BlendMode::Multiply, .5f);
  composite_node->position = {620, 220};
  if (!composite_node->initialize(400, 300)) {
    throw std::runtime_error("Unable to start");
  }
  const auto composite = graph.add_node(std::move(composite_node));
  graph.add_link(color->outputs[0], composite->inputs[0]);
  graph.add_link(grad->outputs[0], composite->inputs[1]);

  // Visual: OutputNode (sink) — receives the composited texture
  graph.add_link(composite->outputs[0], output_node->inputs[0]);

  // Set stream pointers
  noise_stream_out = dynamic_cast<Stream<float> *>(noise_ptr->outputs[0].stream.get());
  sin_a_stream_out = dynamic_cast<Stream<float> *>(sin_a->outputs[0].stream.get());
  sin_b_stream_out = dynamic_cast<Stream<float> *>(lfo_b->outputs[0].stream.get());
  out_stream = dynamic_cast<Stream<float> *>(adding->outputs[0].stream.get());
}

// ============================================================================
// Graph Actions
// ============================================================================

void GraphVisualInsight::reset_graph() {
  // invalidate pointers to clear references
  time_node = nullptr;
  output_node = nullptr;

  noise_stream_out = nullptr;
  sin_a_stream_out = nullptr;
  sin_b_stream_out = nullptr;
  out_stream = nullptr;

  graph.clear();
  node_pos_refresh = true;
}

void GraphVisualInsight::save_graph(const std::string &path) {
  if (const auto result = serializer.save(graph, path)) {
    status_message = ICON_FA_CHECK "  Graph saved to " + path;
    spdlog::info("Graph saved to {}", path);
  } else {
    status_message = ICON_FA_TRIANGLE_EXCLAMATION "  Save failed: " + result.error_message;
    spdlog::error("Graph save failed: {}", result.error_message);
  }

  status_message_time = static_cast<float>(ImGui::GetTime());
}

// ============================================================================
// delete_selected_nodes
// ============================================================================

void GraphVisualInsight::delete_selected_nodes() {
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

  // ---- Step 1: Invalidate plot stream pointers if deleted ---------------
  for (const auto &node: graph.nodes) {
    if (!nodes_to_delete.contains(node->id)) continue;

    // Check if this node is connected to our plot streams
    for (const auto &output: node->outputs) {
      if (output.stream.get() == noise_stream_out ||
          output.stream.get() == sin_a_stream_out ||
          output.stream.get() == sin_b_stream_out ||
          output.stream.get() == out_stream) {
        // Null out the pointer to prevent dangling reference
        if (output.stream.get() == noise_stream_out) noise_stream_out = nullptr;
        if (output.stream.get() == sin_a_stream_out) sin_a_stream_out = nullptr;
        if (output.stream.get() == sin_b_stream_out) sin_b_stream_out = nullptr;
        if (output.stream.get() == out_stream) out_stream = nullptr;
      }
    }
  }

  delete_nodes(nodes_to_delete);
}

// ============================================================================
// delete_selected_links
// ============================================================================

void GraphVisualInsight::delete_selected_links() {
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

void GraphVisualInsight::render_ui() {
  render_node_editor();

  render_plot();
}

// ============================================================================
// render_menu_bar
// ============================================================================

void GraphVisualInsight::render_menu_bar() {
  if (!ImGui::BeginMenuBar()) return;

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
      status_message = ICON_FA_CHECK "  Graph reset to default";
      status_message_time = static_cast<float>(ImGui::GetTime());
    }

    ImGui::EndMenu();
  }

  // ── File Menu ──────────────────────────────────────────────────────────────
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

  // ── View menu ─────────────────

  if (ImGui::BeginMenu(ICON_FA_TV "  View")) {
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

    // Pause toggle — enabled only if the preview is open.
    if (ImGui::MenuItem(ICON_FA_PAUSE "  Pause Preview",
                        "Space",
                        preview_window.is_paused(),
                        preview_window.is_open())) {
      preview_window.toggle_pause();
    }

    ImGui::EndMenu();
  }


  // ── Status Message (fade out after kStatusMessageDuration) ─────────────────
  const float elapsed = static_cast<float>(ImGui::GetTime()) - status_message_time;
  if (!status_message.empty() && elapsed < kStatusMessageDuration) {
    // Alpha: 100% for 2s, then 1s fade out
    constexpr float fade_start = kStatusMessageDuration - 1.f;
    const float alpha = elapsed > fade_start
                          ? 1.f - (elapsed - fade_start)
                          : 1.f;

    ImGui::SameLine(0.f, 30.f);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImVec4(0.6f, 1.f, 0.6f, alpha));
    ImGui::TextUnformatted(status_message.c_str());
    ImGui::PopStyleColor();
  }

  ImGui::EndMenuBar();

  // ── File Dialog: Save ──────────────────────────────────────────────────────
  constexpr ImVec2 dialog_size{600.f, 400.f};
  if (ImGuiFileDialog::Instance()->Display(
    kSaveGraphDialogKey, ImGuiWindowFlags_NoCollapse, dialog_size)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      save_graph(ImGuiFileDialog::Instance()->GetFilePathName());
    }
    ImGuiFileDialog::Instance()->Close();
  }

  // ── File Dialog: Load ──────────────────────────────────────────────────────
  if (ImGuiFileDialog::Instance()->Display(
    kLoadGraphDialogKey, ImGuiWindowFlags_NoCollapse, dialog_size)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const auto path = ImGuiFileDialog::Instance()->GetFilePathName();
      if (auto result = load_graph(path); !result) {
        status_message = ICON_FA_TRIANGLE_EXCLAMATION "  Load failed: " +
                         result.error_message;
        status_message_time = static_cast<float>(ImGui::GetTime());
      } else {
        // Stream pointers become null: empty plot after load
        status_message = ICON_FA_CHECK "  Graph loaded from " + path;
        status_message_time = static_cast<float>(ImGui::GetTime());
        spdlog::info("Graph loaded from {}", path);
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }
}

// ============================================================================
// render_plot
// ============================================================================

void GraphVisualInsight::render_plot() {
  static ScrollingBuffer buf_noise(1000);
  static ScrollingBuffer buf_a(1000);
  static ScrollingBuffer buf_b(1000);
  static ScrollingBuffer buf_out(1000);

  if (ImGui::Begin("Time plot")) {
    // Toolbar plot
    if (!plot_flowing) {
      if (ImGui::Button(ICON_FA_PLAY "##play")) plot_flowing = true;
    } else if (ImGui::Button(ICON_FA_PAUSE "##pause")) {
      plot_flowing = false;
    }

    ImGui::SameLine();

    const float display_time = time_node ? time_node->time : 0.f;
    ImGui::Text("Time: %.2fs", display_time);

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
      if (time_node) time_node->time = 0.f;
      buf_noise.Erase();
      buf_a.Erase();
      buf_b.Erase();
      buf_out.Erase();
    }

    ImGui::SameLine();
    ImGui::SliderFloat("History", &plot_history, 1.f, 30.f);

    // Tick
    if (plot_flowing && time_node) {
      time_node->step(ImGui::GetIO().DeltaTime);
      graph.evaluate();

      // Add points only of ptrs are still valid (null after load)
      if (noise_stream_out)
        buf_noise.AddPoint(time_node->time, noise_stream_out->value);
      if (sin_a_stream_out)
        buf_a.AddPoint(time_node->time, sin_a_stream_out->value);
      if (sin_b_stream_out)
        buf_b.AddPoint(time_node->time, sin_b_stream_out->value);
      if (out_stream)
        buf_out.AddPoint(time_node->time, out_stream->value);
    }

    // Plot
    static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;
    static ImPlotSpec spec;
    spec.Size = 0;
    spec.Stride = 2 * sizeof(float);

    if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
      ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
      ImPlot::SetupAxisLimits(
        ImAxis_X1,
        std::max(display_time - plot_history, 0.f),
        std::max(plot_history, display_time),
        ImGuiCond_Always
      );
      ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);

      auto plot_line = [&](const char *label, ScrollingBuffer &buf) {
        if (buf.Data.empty()) return;
        spec.Offset = buf.Offset;
        ImPlot::PlotLine(label,
                         &buf.Data[0].x, &buf.Data[0].y,
                         buf.Data.size(), spec);
      };

      plot_line("Noise", buf_noise);
      plot_line("Sin A", buf_a);
      plot_line("Sin B", buf_b);
      plot_line("Sum", buf_out);

      ImPlot::EndPlot();
    }
  }

  ImGui::End(); // Time plot
}

// ============================================================================
// render_node_editor
// ============================================================================

void GraphVisualInsight::render_node_editor() {
  if (ImGui::Begin("Node Editor", nullptr, ImGuiWindowFlags_MenuBar)) {
    render_menu_bar();

    ImNodes::EditorContextSet(editor_context);
    ImNodes::BeginNodeEditor();

    const bool refresh_positions = node_pos_refresh.exchange(false);

    // ── Nodes ──────────────────────────────────────────────────────────────────
    for (const auto &node: graph.nodes) {
      // ReSharper disable once CppDFAConstantConditions
      if (refresh_positions) {
        ImNodes::SetNodeScreenSpacePos(node->id, node->position);
      }

      ImNodes::BeginNode(node->id);

      ImNodes::BeginNodeTitleBar();
      ImGui::TextUnformatted(node->name.c_str());
      ImNodes::EndNodeTitleBar();

      if (node->inputs.size() + node->outputs.size() == 0) {
        ImGui::TextUnformatted("No inputs and outputs?!");
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
      ImNodes::Link(id, start_pin_id, end_pin_id
      );
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
    if (ImNodes::IsLinkCreated(
      &start_pin_id,
      &end_pin_id)) {
      auto add_link_command = std::make_unique<AddLinkCommand>(
        start_pin_id, end_pin_id);

      command_history.execute(graph, std::move(add_link_command));
    }

    // ── Link Deletion ──────────────────────────────────────────────────────────
    int link_id;
    if (ImNodes::IsLinkDestroyed(&link_id)) {
      for (const auto &link: graph.links) {
        if (link.id == link_id) {
          for (const auto &node: graph.nodes) {
            for (auto &pin: node->inputs) {
              if (pin.id == link.end_pin_id) {
                pin.stream = nullptr;
                goto link_cleanup_done;
              }
            }
          }
        link_cleanup_done:
          break;
        }
      }

      std::erase_if(graph.links, [link_id](const Link &link) {
        return link.id == link_id;
      });
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
          status_message = ICON_FA_TRASH "  Deleted " + std::to_string(node_no) +
                           (node_no == 1 ? " node" : " nodes") + " and " + std::to_string(link_no) +
                           (link_no == 1 ? " link" : " links");
          status_message_time = static_cast<float>(ImGui::GetTime());
        }
      }

      // TODO ctrl-z


      // Preview pause toggle
      if (ImGui::IsKeyPressed(ImGuiKey_Space) && preview_window.is_open()) {
        preview_window.toggle_pause();
      }
    }
  }
  ImGui::End(); // Node editor

  if (ImGui::Begin("Node properties")) {
    node_properties_panel_.render(graph, command_history);
  }
  ImGui::End(); // Node properties
}

// ============================================================================
// render_context_menu
// ============================================================================

void GraphVisualInsight::render_context_menu() {
  if (ImGui::BeginPopup("add_node_popup")) {
    // Screen space
    const ImVec2 spawn_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

    ImGui::TextDisabled(ICON_FA_PLUS "  Add Node");
    ImGui::Separator();

    // Get registry categories
    const auto categories = NodeRegistry::instance().get_nodes_by_category();

    // View order
    // @todo use integer constants
    static constexpr const char *const kCategoryOrder[5] = {
      "Visual", "Effects", "Math", "Temporal", "Generators"
    };

    // Category icons
    // @todo dont use a lambda for this trivial task
    auto category_icon = [](const std::string &cat) -> const char * {
      if (cat == "Visual") return ICON_FA_PAINTBRUSH "  ";
      if (cat == "Effects") return ICON_FA_WAND_MAGIC_SPARKLES "  ";
      if (cat == "Math") return ICON_FA_CALCULATOR "  ";
      if (cat == "Temporal") return ICON_FA_CLOCK "  ";
      if (cat == "Generators") return ICON_FA_BOLT "  ";
      return ICON_FA_CIRCLE_NODES "  ";
    };

    // Show categories in the predefined order (and additional ones, if any, afterward)
    auto render_category = [&](const std::string &category) {
      const auto it = categories.find(category);
      if (it == categories.end()) return;

      const std::string label =
          std::string(category_icon(category)) + category;

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
        status_message = "Deleted " + std::to_string(num_nodes) + (num_nodes == 1 ? " node" : " nodes");
        status_message_time = static_cast<float>(ImGui::GetTime());
      }
    }

    if (num_links > 0) {
      const std::string label = num_links == 1
                                  ? ICON_FA_LINK_SLASH "  Delete Link"
                                  : ICON_FA_LINK_SLASH "  Delete " + std::to_string(num_links) + " Links";

      if (ImGui::MenuItem(label.c_str(), "Del")) {
        delete_selected_links();
        status_message = "Deleted " + std::to_string(num_links) + (num_links == 1 ? " link" : " links");
        status_message_time = static_cast<float>(ImGui::GetTime());
      }
    }

    ImGui::EndPopup();
  } // delete selection popup
}

// ============================================================================
// Helpers
// ============================================================================

void GraphVisualInsight::render_visual_node_body(
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

unsigned int GraphVisualInsight::get_pin_color(const Pin &pin) {
  if (*pin.data_type == typeid(float)) return ImColor(100, 200, 100);
  if (*pin.data_type == typeid(Texture *)) return ImColor(200, 100, 200);
  if (*pin.data_type == typeid(void)) return ImColor(150, 150, 150);
  return ImColor(100, 100, 200);
}

int main(int, char **) {
  return GraphVisualInsight().run();
}
