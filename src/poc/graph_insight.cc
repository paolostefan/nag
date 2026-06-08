#include "graph_insight.h"

#include "IconsFontAwesome6.h"
#include "implot.h"

#include "editor/scrolling_buffer.h"


GraphInsight::GraphInsight() : UIWindow("Graph insight POC", 800, 600) {
  auto time_node_unique = TimeNode::create();
  time_node = dynamic_cast<TimeNode *>(graph.add_node(std::move(time_node_unique)));

  auto sin_a_ptr = SinNode::create();
  auto sin_b_ptr = CosNode::create();
  auto adding_node_ptr = AddNode::create();

  sin_a_ptr->name = "Sin A";
  sin_b_ptr->name = "Sin B";

  sin_a = dynamic_cast<SinNode *>(graph.add_node(std::move(sin_a_ptr)));
  sin_b = dynamic_cast<CosNode *>(graph.add_node(std::move(sin_b_ptr)));
  adding_node = dynamic_cast<AddNode *>(graph.add_node(std::move(adding_node_ptr)));


  graph.add_link(time_node->outputs[0], sin_a->inputs[0]);
  graph.add_link(time_node->outputs[0], sin_b->inputs[0]);

  graph.add_link(sin_a->outputs[0], adding_node->inputs[0]);
  graph.add_link(sin_b->outputs[0], adding_node->inputs[1]);

  // ===============

  // Set stream pointers
  sin_a_stream_out = static_cast<Stream *>(sin_a->outputs[0].stream.get());
  sin_b_stream_out = static_cast<Stream *>(sin_b->outputs[0].stream.get());
  out_stream = static_cast<Stream *>(adding_node->outputs[0].stream.get());
}


void GraphInsight::render_ui() {
  ImGui::Begin("Graph Insight", nullptr, ImGuiWindowFlags_NoDecoration);

  static ScrollingBuffer buffer_in_a(1000);
  static ScrollingBuffer buffer_in_b(1000);
  static ScrollingBuffer buffer_out(1000);

  static bool flow = true;
  static float history = 10.f;

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
    time_node->time = 0.f;
    buffer_in_a.Erase();
    buffer_in_b.Erase();
    buffer_out.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &history, 1.f, 30.f);

  if (flow) {
    time_node->step(ImGui::GetIO().DeltaTime);

    graph.evaluate();

    buffer_in_a.AddPoint(time_node->time, *sin_a_stream_out->as_float());
    buffer_in_b.AddPoint(time_node->time, *sin_b_stream_out->as_float());
    buffer_out.AddPoint(time_node->time, *out_stream->as_float());
  }

  static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;
  static ImPlotSpec spec;
  spec.Size = 0;
  spec.Stride = 2 * sizeof(float);

  if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
    ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
    ImPlot::SetupAxisLimits(ImAxis_X1,
                            std::max(time_node->time - history, 0.f), std::max(history, time_node->time),
                            ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);

    auto plot_line = [&](const char *label, ScrollingBuffer &buf) {
      if (buf.Data.empty()) return;
      spec.Offset = buf.Offset;
      ImPlot::PlotLine(label,
                       &buf.Data[0].x, &buf.Data[0].y,
                       buf.Data.size(), spec);
    };

    plot_line("Input A", buffer_in_a);
    plot_line("Input B", buffer_in_b);
    plot_line("Output (A+B)", buffer_out);

    ImPlot::EndPlot();
  }

  for (auto const &node: graph.nodes) {
    ImGui::Begin(node->name.c_str());

    ImGui::Text("Inputs:");
    for (auto const &pin: node->inputs) {
      ImGui::BulletText("%s: %s (v=%lu)", pin.name.c_str(),
                        pin.stream ? data_type_name(pin.data_type) : "null",
                        pin.stream ? pin.stream->version : 0);
    }
    ImGui::Separator();

    ImGui::Text("Outputs:");
    for (auto const &pin: node->outputs) {
      ImGui::BulletText("%s: %s (v=%lu)", pin.name.c_str(),
                        pin.stream ? data_type_name(pin.data_type) : "null",
                        pin.stream ? pin.stream->version : 0);
    }


    ImGui::End();
  }

  ImGui::End();
}

int main(int, char **) {
  return GraphInsight().run();
}
