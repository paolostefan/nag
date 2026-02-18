#include "graph_visual_insight.h"

#include <algorithm>

#include "IconsFontAwesome6.h"
#include "ImGuiFileDialog.h"
#include "implot.h"
#include "spdlog/spdlog.h"

#include "engine/math_nodes.h"
#include "engine/node_registry.h"
#include "engine/temporal_nodes.h"
#include "editor/scrolling_buffer.h"
#include "engine/shader_quad_helper.h"
#include "engine/visual_nodes.h"


GraphVisualInsight::GraphVisualInsight() : UIWindow("Graph insight POC", 1024, 768) {
  // Init stuff
  ShaderQuadHelper::instance().initialize();
  register_all_builtin_nodes();
  build_default_graph();

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


// hardcoded default graph
void GraphVisualInsight::build_default_graph() {
  reset_graph();

  // Time node
  auto time_node_unique = TimeNode::create();
  time_node_unique->position = {40, 100};
  time_node = dynamic_cast<TimeNode *>(graph.add_node(std::move(time_node_unique)));

  // Noise node
  auto noise_node = NoiseNode::create(1.0f, 4.0f, 3, 0.8f);
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
  auto lfo_b_node = LFONode::create(.7f, 1.0f, LFONode::WaveShape::Sine, 0, .5f);
  lfo_b_node->name = "Oscillator B";
  lfo_b_node->position = {150, 190};
  const Node *lfo_b = graph.add_node(std::move(lfo_b_node));
  graph.add_link(time_node->outputs[0], lfo_b->inputs[0]);

  // Adding node connected to A, B and Noise
  auto adding_node = AddFloatNode::create(3);
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
  noise_stream_out = nullptr;
  sin_a_stream_out = nullptr;
  sin_b_stream_out = nullptr;
  out_stream = nullptr;

  graph.clear();
  first_render = true;
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

void GraphVisualInsight::load_graph(const std::string &path) {
  // load a temp graph
  NodeGraph temp_graph;

  if (const auto result = serializer.load(temp_graph, path); !result) {
    status_message = ICON_FA_TRIANGLE_EXCLAMATION "  Load failed: " + result.error_message;
    status_message_time = static_cast<float>(ImGui::GetTime());
    spdlog::error("Graph load failed: {}", result.error_message);
    return;
  }

  // Commit: replace current graph
  reset_graph();
  graph = std::move(temp_graph);

  // search time node in the existing graph
  for (const auto &node: graph.nodes) {
    if (node->type == Time) {
      time_node = dynamic_cast<TimeNode *>(node.get());
      break;
    }
  }

  // Stream pointers become null: empty plot after load
  status_message = ICON_FA_CHECK "  Graph loaded from " + path;
  status_message_time = static_cast<float>(ImGui::GetTime());
  spdlog::info("Graph loaded from {}", path);
}

void GraphVisualInsight::spawn_node(NodeType type, const ImVec2 &position) {
  auto node = NodeRegistry::instance().create_node(type);
  if (!node) {
    spdlog::error("Failed to create node of type {}", static_cast<int>(type));
    return;
  }

  // Init visual nodes with default sie
  if (auto *visual = dynamic_cast<VisualNode *>(node.get())) {
    if (!visual->initialize(400, 300)) {
      spdlog::warn("Visual node '{}' failed to initialize render target",
                   node->name);
    }
  }

  node->position = position;
  graph.add_node(std::move(node));
}

// ============================================================================
// render_ui
// ============================================================================

void GraphVisualInsight::render_ui() {
  ImGui::Begin("Graph Insight", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);

  render_menu_bar();
  render_plot();
  render_node_editor();

  ImGui::End();
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
        kSaveDialogKey, "Save Graph", kFileFilter, cfg
      );
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Load Graph", "Ctrl+O")) {
      IGFD::FileDialogConfig cfg;
      cfg.path = ".";
      ImGuiFileDialog::Instance()->OpenDialog(
        kLoadDialogKey, "Load Graph", kFileFilter, cfg
      );
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ICON_FA_ROTATE_LEFT "  Reset to Default")) {
      build_default_graph();
      status_message = ICON_FA_CHECK "  Graph reset to default";
      status_message_time = static_cast<float>(ImGui::GetTime());
    }

    ImGui::EndMenu();
  }

  // ── Status Message (fade out dopo kStatusMessageDuration) ─────────────────
  const float elapsed = static_cast<float>(ImGui::GetTime()) - status_message_time;
  if (!status_message.empty() && elapsed < kStatusMessageDuration) {
    // Alpha: 100% for 2s, then 1s fade out
    constexpr float fade_start = kStatusMessageDuration - 1.0f;
    const float alpha = elapsed > fade_start
                          ? 1.0f - (elapsed - fade_start)
                          : 1.0f;

    ImGui::SameLine(0.0f, 30.0f);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImVec4(0.6f, 1.0f, 0.6f, alpha));
    ImGui::TextUnformatted(status_message.c_str());
    ImGui::PopStyleColor();
  }

  ImGui::EndMenuBar();

  // ── File Dialog: Save ──────────────────────────────────────────────────────
  constexpr ImVec2 dialog_size{600.0f, 400.0f};
  if (ImGuiFileDialog::Instance()->Display(
    kSaveDialogKey, ImGuiWindowFlags_NoCollapse, dialog_size)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      save_graph(ImGuiFileDialog::Instance()->GetFilePathName());
    }
    ImGuiFileDialog::Instance()->Close();
  }

  // ── File Dialog: Load ──────────────────────────────────────────────────────
  if (ImGuiFileDialog::Instance()->Display(
    kLoadDialogKey, ImGuiWindowFlags_NoCollapse, dialog_size)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      load_graph(ImGuiFileDialog::Instance()->GetFilePathName());
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

  // Toolbar plot
  if (!plot_flowing) {
    if (ImGui::Button(ICON_FA_PLAY "##play")) plot_flowing = true;
  } else if (ImGui::Button(ICON_FA_PAUSE "##pause")) {
    plot_flowing = false;
  }

  ImGui::SameLine();

  const float display_time = time_node ? time_node->time : 0.0f;
  ImGui::Text("Time: %.2fs", display_time);

  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    if (time_node) time_node->time = 0.0f;
    buf_noise.Erase();
    buf_a.Erase();
    buf_b.Erase();
    buf_out.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &plot_history, 1.0f, 30.0f);

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
      std::max(display_time - plot_history, 0.0f),
      std::max(plot_history, display_time),
      ImGuiCond_Always
    );
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);

    auto plot_line = [&](const char *label, ScrollingBuffer &buf) {
      if (buf.Data.empty()) return;
      spec.Offset = buf.Offset;
      ImPlot::PlotLine(label,
                       &buf.Data[0].x, &buf.Data[0].y,
                       static_cast<int>(buf.Data.size()), spec);
    };

    plot_line("Noise", buf_noise);
    plot_line("Sin A", buf_a);
    plot_line("Sin B", buf_b);
    plot_line("Sum", buf_out);

    ImPlot::EndPlot();
  }
}

// ============================================================================
// render_node_editor
// ============================================================================

void GraphVisualInsight::render_node_editor() {
  ImNodes::EditorContextSet(editor_context);
  ImGui::Begin("Node Editor");
  ImNodes::BeginNodeEditor();

  // ── Nodes ──────────────────────────────────────────────────────────────────
  for (const auto &node: graph.nodes) {
    // ReSharper disable once CppDFAConstantConditions
    if (first_render) {
      ImNodes::SetNodeScreenSpacePos(
        static_cast<int>(node->id), node->position
      );
    }

    ImNodes::BeginNode(static_cast<int>(node->id));

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(node->name.c_str());
    ImNodes::EndNodeTitleBar();

    if (node->inputs.size() + node->outputs.size() == 0) {
      ImGui::TextUnformatted("No inputs and outputs?!");
    }

    for (const auto &pin: node->inputs) {
      ImNodes::PushColorStyle(ImNodesCol_Pin, get_pin_color(pin));
      ImNodes::BeginInputAttribute(static_cast<int>(pin.id));
      ImGui::TextUnformatted(pin.name.c_str());
      ImNodes::EndInputAttribute();
      ImNodes::PopColorStyle();
    }

    if (const auto *visual_node = dynamic_cast<VisualNode *>(node.get())) {
      render_visual_node_body(visual_node);
    }

    for (const auto &pin: node->outputs) {
      ImNodes::PushColorStyle(ImNodesCol_Pin, get_pin_color(pin));
      ImNodes::BeginOutputAttribute(static_cast<int>(pin.id));
      ImGui::TextUnformatted(pin.name.c_str());
      ImNodes::EndOutputAttribute();
      ImNodes::PopColorStyle();
    }

    ImNodes::EndNode();
  }

  first_render = false;

  // ── Links ──────────────────────────────────────────────────────────────────
  for (const auto &[id, start_pin_id, end_pin_id]: graph.links) {
    ImNodes::Link(
      static_cast<int>(id),
      static_cast<int>(start_pin_id),
      static_cast<int>(end_pin_id)
    );
  }

  ImNodes::EndNodeEditor();

  // ── Detect Right Click on Canvas ───────────────────────────────────────────
  // Must come after EndNodeEditor() and before OpenPopup()

  static int hovered_node_id{0}, hovered_link_id{0};

  const bool should_open_context_menu =
      ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
      !ImNodes::IsNodeHovered(&hovered_node_id) &&
      !ImNodes::IsLinkHovered(&hovered_link_id);
  if (should_open_context_menu) {
    ImGui::OpenPopup("add_node_popup");
  }

  // ── Context Menu ───────────────────────────────────────────────────────────
  render_context_menu();

  // ── Link Creation ──────────────────────────────────────────────────────────
  int start_pin_id, end_pin_id;
  if (ImNodes::IsLinkCreated(&start_pin_id, &end_pin_id)) {
    graph.add_link(start_pin_id, end_pin_id);
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

  ImGui::End();
}

// ============================================================================
// render_context_menu
// ============================================================================

void GraphVisualInsight::render_context_menu() {
  if (!ImGui::BeginPopup("add_node_popup")) return;

  // Screen space
  const ImVec2 popup_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

  // Position on imnodes canvas where to put the popup
  const ImVec2 spawn_pos = popup_pos; // ImNodes::ScreenSpaceToGridSpace(popup_pos);;

  ImGui::TextDisabled(ICON_FA_PLUS "  Add Node");
  ImGui::Separator();

  // Get registry categories
  const auto categories = NodeRegistry::instance().get_nodes_by_category();

  // View order
  static constexpr std::array<const char *, 4> kCategoryOrder{
    "Visual", "Math", "Temporal", "Generators"
  };

  // Category icons
  auto category_icon = [](const std::string &cat) -> const char * {
    if (cat == "Visual") return ICON_FA_PAINTBRUSH "  ";
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

  constexpr float kPreviewWidth = 150.0f;
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
