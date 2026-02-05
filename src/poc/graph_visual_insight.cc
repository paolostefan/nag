#include "graph_visual_insight.h"

#include <algorithm>

#include "IconsFontAwesome6.h"
#include "implot.h"

#include "editor/scrolling_buffer.h"


GraphVisualInsight::GraphVisualInsight() : UIWindow("Graph insight POC", 800, 600) {
  time_stream = graph.create_external_stream<float>();

  auto sin_a_node = std::make_unique<SinNode>(3., 2);

  sin_a_node->add_input("time");
  sin_a_node->add_output("Sin A");
  sin_a_node->name = "Sin A";
  sin_a_node->position = {10, 30};
  Node *sin_a = graph.add_node(std::move(sin_a_node));
  sin_a->inputs[0].stream = time_stream;

  auto sin_b_node = std::make_unique<SinNode>(1.7, 15.1, 1);
  sin_b_node->add_input("time");
  sin_b_node->add_output("Sin B");
  sin_b_node->name = "Sin B";
  sin_b_node->position = {10, 150};
  Node *sin_b = graph.add_node(std::move(sin_b_node));
  sin_b->inputs[0].stream = time_stream;

  auto adding_node = std::make_unique<AddFloatNode>();
  adding_node->position = {250, 90};
  adding_node->add_input();
  adding_node->add_input();
  adding_node->add_output("sum");
  Node *adding = graph.add_node(std::move(adding_node));

  graph.add_link(sin_a->outputs[0], adding->inputs[0]);
  graph.add_link(sin_b->outputs[0], adding->inputs[1]);


  // ===============

  // Set stream pointers
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
    buffer_in_a.Erase();
    buffer_in_b.Erase();
    buffer_out.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &history, 1.0f, 30.0f);

  if (flow) {
    time_stream->update(time_stream->value + ImGui::GetIO().DeltaTime);

    graph.evaluate();

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

    if (!buffer_in_a.Data.empty()) {
      ImPlot::PlotLine("Input A",
                       &buffer_in_a.Data[0].x, &buffer_in_a.Data[0].y,
                       buffer_in_a.Data.size(),
                       0, buffer_in_a.Offset, 2 * sizeof(float));
    }

    if (!buffer_in_b.Data.empty()) {
      ImPlot::PlotLine("Input B",
                       &buffer_in_b.Data[0].x, &buffer_in_b.Data[0].y,
                       buffer_in_b.Data.size(),
                       0, buffer_in_b.Offset, 2 * sizeof(float));
    }

    if (!buffer_out.Data.empty()) {
      ImPlot::PlotLine("Output (A+B)",
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
