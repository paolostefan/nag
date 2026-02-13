#include "graph_visual_insight.h"

#include <algorithm>

#include "IconsFontAwesome6.h"
#include "implot.h"

#include "engine/math_nodes.h"
#include "engine/temporal_nodes.h"
#include "editor/scrolling_buffer.h"
#include "engine/visual_nodes.h"


GraphVisualInsight::GraphVisualInsight() : UIWindow("Graph insight POC", 800, 600) {
  // Create nodes

  // Time node
  auto time_node_unique = TimeNode::create();
  time_node_unique->position = {40, 100};
  time_node = dynamic_cast<TimeNode *>(graph.add_node(std::move(time_node_unique)));

  // Noise node connected to time
  auto noise_node = NoiseNode::create(1.0f,
                                      4.0f,
                                      3,
                                      0.8f);
  noise_node->position = {150, 30};
  const auto noise_node_ptr = graph.add_node(std::move(noise_node));
  graph.add_link(time_node->outputs[0], noise_node_ptr->inputs[0]);

  // Oscillator connected to time
  auto lfo_node = LFONode::create(3.0f,
                                    2.5f,
                                    LFONode::WaveShape::Sawtooth);
  lfo_node->name = "Oscillator A";
  lfo_node->position = {150, 110};
  const Node *sin_a = graph.add_node(std::move(lfo_node));
  graph.add_link(time_node->outputs[0], sin_a->inputs[0]);

  // Oscillator B node connected to time
  auto lfo_b_node = LFONode::create(5.7f,
                                    1.0f,
                                    LFONode::WaveShape::Sine,
                                    0,
                                    .5f);
  lfo_b_node->name = "Oscillator B";
  lfo_b_node->position = {150, 190};
  const Node *lfo_b = graph.add_node(std::move(lfo_b_node));
  graph.add_link(time_node->outputs[0], lfo_b->inputs[0]);

  // Adding node connected to A, B and Noise
  auto adding_node = AddFloatNode::create(3);
  adding_node->position = {290, 30};
  const Node *adding = graph.add_node(std::move(adding_node));

  // Visual node
  auto color_node = ClearColorNode::create(Color::red());
  if (!color_node->initialize(400,300)) {
    throw std::runtime_error("Unable to start");
  }

  color_node->position = {290, 120};
  const Node *color = graph.add_node(std::move(color_node));

  // Link stuff together

  graph.add_link(noise_node_ptr->outputs[0], adding->inputs[0]);
  graph.add_link(sin_a->outputs[0], adding->inputs[1]);
  graph.add_link(lfo_b->outputs[0], adding->inputs[2]);
  graph.add_link(lfo_b->outputs[0], color->inputs[2] /* The blue component will oscillate */);


  // ===============

  // Set stream pointers
  noise_stream_out = dynamic_cast<Stream<float> *>(noise_node_ptr->outputs[0].stream.get());
  sin_a_stream_out = dynamic_cast<Stream<float> *>(sin_a->outputs[0].stream.get());
  sin_b_stream_out = dynamic_cast<Stream<float> *>(lfo_b->outputs[0].stream.get());
  out_stream = dynamic_cast<Stream<float> *>(adding->outputs[0].stream.get());

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


void GraphVisualInsight::render_ui() {
  ImGui::Begin("Graph Insight", nullptr, ImGuiWindowFlags_NoDecoration);

  static ScrollingBuffer buffer_in_noise(1000);
  static ScrollingBuffer buffer_in_a(1000);
  static ScrollingBuffer buffer_in_b(1000);
  static ScrollingBuffer buffer_out(1000);

  static bool flow = true;
  static float history = 10.0f;


  if (!flow) {
    if (ImGui::Button(ICON_FA_PLAY "##play")) {
      flow = true;
    }
  } else if (ImGui::Button(ICON_FA_PAUSE "##pause")) {
    flow = false;
  }

  ImGui::SameLine();
  ImGui::Text("Time: %.2fs", time_node->time);
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    time_node->time = 0.0f;
    buffer_in_noise.Erase();
    buffer_in_a.Erase();
    buffer_in_b.Erase();
    buffer_out.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &history, 1.0f, 30.0f);

  if (flow) {
    time_node->step(ImGui::GetIO().DeltaTime);

    graph.evaluate();

    buffer_in_noise.AddPoint(time_node->time, noise_stream_out->value);
    buffer_in_a.AddPoint(time_node->time, sin_a_stream_out->value);
    buffer_in_b.AddPoint(time_node->time, sin_b_stream_out->value);
    buffer_out.AddPoint(time_node->time, out_stream->value);
  }

  static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;

  if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
    ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
    ImPlot::SetupAxisLimits(ImAxis_X1,
                            std::max(time_node->time - history, 0.0f),
                            std::max(history, time_node->time),
                            ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
    if (!buffer_in_noise.Data.empty()) {
      ImPlot::PlotLine("Noise",
                       &buffer_in_noise.Data[0].x, &buffer_in_noise.Data[0].y,
                       buffer_in_noise.Data.size(),
                       0, buffer_in_noise.Offset, 2 * sizeof(float));
    }

    if (!buffer_in_a.Data.empty()) {
      ImPlot::PlotLine("Sin A",
                       &buffer_in_a.Data[0].x, &buffer_in_a.Data[0].y,
                       buffer_in_a.Data.size(),
                       0, buffer_in_a.Offset, 2 * sizeof(float));
    }

    if (!buffer_in_b.Data.empty()) {
      ImPlot::PlotLine("Sin B",
                       &buffer_in_b.Data[0].x, &buffer_in_b.Data[0].y,
                       buffer_in_b.Data.size(),
                       0, buffer_in_b.Offset, 2 * sizeof(float));
    }

    if (!buffer_out.Data.empty()) {
      ImPlot::PlotLine("Sum",
                       &buffer_out.Data[0].x, &buffer_out.Data[0].y,
                       buffer_out.Data.size(),
                       0, buffer_out.Offset, 2 * sizeof(float));
    }

    ImPlot::EndPlot();
  }

  render_node_editor();

  ImGui::End();
}

void GraphVisualInsight::render_node_editor() {

  ImNodes::EditorContextSet(editor_context);
  ImGui::Begin("Node Editor");
  ImNodes::BeginNodeEditor();

  static bool first_render = true;

  for (const auto &node: graph.nodes) {
    if (first_render) {
      ImNodes::SetNodeScreenSpacePos(static_cast<int>(node->id), node->position);
    }

    ImNodes::BeginNode(static_cast<int>(node->id));

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(node->name.c_str());
    ImNodes::EndNodeTitleBar();

    for (const auto &pin: node->inputs) {
      ImNodes::BeginInputAttribute(static_cast<int>(pin.id));
      ImGui::TextUnformatted(pin.name.c_str());
      ImNodes::EndInputAttribute();
    }

    if (auto *visual_node = dynamic_cast<VisualNode *>(node.get())) {
      render_visual_node_body(visual_node);
    }

    // switch (node->type) {
    //   case Time: {
    //     const auto *_stream = dynamic_cast<Stream<float> *>(node->outputs[0].stream);
    //     ImGui::Text("%.3f", _stream ? _stream->value : 0.0f);
    //   }
    //   break;
    //   case Sin:
    //     ImGui::TextUnformatted("Sin");
    //     break;
    //   default:
    //     ImGui::TextUnformatted("Unknown");
    //     break;
    // }

    for (const auto &pin: node->outputs) {
      ImNodes::BeginOutputAttribute(static_cast<int>(pin.id));
      ImGui::TextUnformatted(pin.name.c_str());
      ImNodes::EndOutputAttribute();
    }

    ImNodes::EndNode();
  }

  // Don't force node position on later renders
  first_render = false;

  for (const auto &[id, start_pin_id, end_pin_id]: graph.links) {
    ImNodes::Link(static_cast<int>(id),
                  static_cast<int>(start_pin_id),
                  static_cast<int>(end_pin_id));
  }

  ImNodes::EndNodeEditor();

  // Handle link creation
  int start_pin_id, end_pin_id;
  if (ImNodes::IsLinkCreated(&start_pin_id, &end_pin_id)) {
    graph.add_link(start_pin_id, end_pin_id);
  }

  // Handle link deletion
  int link_id;
  if (ImNodes::IsLinkDestroyed(&link_id)) {
    for (const auto &link: graph.links) {
      if (link.id == static_cast<uint64_t>(link_id)) {
        // Detach stream from destination pin: i.e., end pin must point to nullptr stream
        bool end_pin_found = false;

        for (const auto &node: graph.nodes) {
          for (auto &pin: node->inputs) {
            if (pin.id == link.end_pin_id) {
              end_pin_found = true;
              pin.stream = nullptr;
              break;
            }
          }

          if (end_pin_found) break;
        }

        if (end_pin_found) break;
      }
    }

    std::erase_if(graph.links, [link_id](const Link &link) {
      return link.id == static_cast<uint64_t>(link_id);
    });
  }

  ImGui::End();
}

void GraphVisualInsight::render_visual_node_body(const VisualNode *visual_node) {
  if (!visual_node || !visual_node->render_target || !visual_node->render_target->is_valid()) {
    ImGui::TextDisabled("(no texture)");
    return;
  }

  const GLuint texture_id = visual_node->render_target->get_texture();

  // Define preview size (adjust to your preference)
  constexpr float preview_width = 200.0f;
  const float aspect_ratio = static_cast<float>(visual_node->render_target->get_height()) /
                             static_cast<float>(visual_node->render_target->get_width());
  const float preview_height = preview_width * aspect_ratio;

  // Center the image in the node
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                       (ImGui::GetContentRegionAvail().x - preview_width) * 0.5f);

  // Draw texture (note: UV coords flipped for OpenGL)
  ImGui::Image(texture_id,
               ImVec2(preview_width, preview_height),
               ImVec2(0, 1),  // UV top-left
               ImVec2(1, 0)); // UV bottom-right
}

int main(int, char **) {
  return GraphVisualInsight().run();
}
