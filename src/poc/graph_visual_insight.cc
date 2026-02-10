#include "graph_visual_insight.h"

#include <algorithm>

#include "IconsFontAwesome6.h"
#include "implot.h"

#include "engine/generator_nodes.h"
#include "editor/scrolling_buffer.h"


GraphVisualInsight::GraphVisualInsight() : UIWindow("Graph insight POC", 800, 600) {
  const auto time_stream = graph.create_external_stream<float>();

  // Create nodes

  // Time node
  auto time_node = std::make_unique<TimeNode>();
  time_node->add_output("time");
  time_node->outputs[0].stream = time_stream;
  time_node->position = {40, 100};
  const auto time_node_ptr = graph.add_node(std::move(time_node));

  // Noise node connected to time
  auto noise_node = std::make_unique<NoiseNode>();
  noise_node->add_input("x");
  noise_node->add_output("noise");
  noise_node->position = {150, 30};
  const auto noise_node_ptr = graph.add_node(std::move(noise_node));
  graph.add_link(time_node_ptr->outputs[0], noise_node_ptr->inputs[0]);

  // Sin A node connected to time
  auto sin_a_node = std::make_unique<SinNode>(3., 2);
  sin_a_node->add_input("time");
  sin_a_node->add_output("Sin A");
  sin_a_node->name = "Sin A";
  sin_a_node->position = {150, 110};
  const Node *sin_a = graph.add_node(std::move(sin_a_node));
  graph.add_link(time_node_ptr->outputs[0], sin_a->inputs[0]);

  // Sin B node connected to time
  auto sin_b_node = std::make_unique<SinNode>(1.7, 15.1, 1);
  sin_b_node->add_input("time");
  sin_b_node->add_output("Sin B");
  sin_b_node->name = "Sin B";
  sin_b_node->position = {150, 190};
  const Node *sin_b = graph.add_node(std::move(sin_b_node));
  graph.add_link(time_node_ptr->outputs[0], sin_b->inputs[0]);

  // Adding node connected to Sin A, Sin B and Noise
  auto adding_node = std::make_unique<AddFloatNode>();
  adding_node->position = {290, 110};
  adding_node->add_input();
  adding_node->add_input();
  adding_node->add_input();
  adding_node->add_output("sum");
  const Node *adding = graph.add_node(std::move(adding_node));

  graph.add_link(noise_node_ptr->outputs[0], adding->inputs[0]);
  graph.add_link(sin_a->outputs[0], adding->inputs[1]);
  graph.add_link(sin_b->outputs[0], adding->inputs[2]);
  // ===============

  // Set stream pointers
  noise_stream_out = dynamic_cast<Stream<float> *>(noise_node_ptr->outputs[0].stream);
  sin_a_stream_out = dynamic_cast<Stream<float> *>(sin_a->outputs[0].stream);
  sin_b_stream_out = dynamic_cast<Stream<float> *>(sin_b->outputs[0].stream);
  out_stream = dynamic_cast<Stream<float> *>(adding->outputs[0].stream);

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

  // Find time stream
  Stream<float> *time_stream = nullptr;

  for (const auto &node: graph.nodes) {
    if (node->type == Time) {
      time_stream = dynamic_cast<Stream<float> *>(node->outputs[0].stream);
      break;
    }
  }

  if (!time_stream) {
    ImGui::Text("No time node found");
    ImGui::End();
    return;
  }


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
  ImGui::Text("Time: %.2fs", time_stream->value);
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    time_stream->update(0.0f);
    buffer_in_noise.Erase();
    buffer_in_a.Erase();
    buffer_in_b.Erase();
    buffer_out.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &history, 1.0f, 30.0f);

  if (flow) {
    time_stream->update(time_stream->value + ImGui::GetIO().DeltaTime);

    graph.evaluate();

    buffer_in_noise.AddPoint(time_stream->value, noise_stream_out->value);
    buffer_in_a.AddPoint(time_stream->value, sin_a_stream_out->value);
    buffer_in_b.AddPoint(time_stream->value, sin_b_stream_out->value);
    buffer_out.AddPoint(time_stream->value, out_stream->value);
  }

  static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;

  if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
    ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
    ImPlot::SetupAxisLimits(ImAxis_X1,
                            std::max(time_stream->value - history, 0.0f), std::max(history, time_stream->value),
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

  // Node editor
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
    for (auto &link: graph.links) {
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
  // End node editor

  ImGui::End();
}

int main(int, char **) {
  return GraphVisualInsight().run();
}
