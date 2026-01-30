#include "node_insight.h"

#include "IconsFontAwesome6.h"
#include "implot.h"

#include "editor/scrolling_buffer.h"

int main(int, char **) {
  return NodeInsight().run();
}

NodeInsight::NodeInsight() : UIWindow("Node Insight", 1280, 720),
                             sin_a(&time_stream, &sin_stream_out, 3.111, 1.75),
                             sin_b(&time_stream, &cos_stream_out, 1.0, 12.1, M_PI / 2.0),
                             adding_node(sin_a.out, sin_b.out, &out_stream) {
  graph = {&sin_a, &sin_b, &adding_node};
}

void NodeInsight::evaluate_node_graph() const {
  bool progress;
  do {
    progress = false;

    for (Node *node: graph) {
      if (node->needs_evaluation()) {
        node->evaluate();
        progress = true;
      }
    }
  } while (progress);
}

void NodeInsight::render_ui() {
  ImGui::Begin("Node Insight");

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
  ImGui::Text("Time: %.2fs", time_stream.value);
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    time_stream.update( 0.0f);
    buffer_in_a.Erase();
    buffer_in_b.Erase();
    buffer_out.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &history, 1.0f, 30.0f);

  if (flow) {
    time_stream.update(time_stream.value + ImGui::GetIO().DeltaTime);

    evaluate_node_graph();

    buffer_in_a.AddPoint(time_stream.value, sin_stream_out.value);
    buffer_in_b.AddPoint(time_stream.value, cos_stream_out.value);
    buffer_out.AddPoint(time_stream.value, out_stream.value);
  }

  static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;

  if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
    ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
    ImPlot::SetupAxisLimits(ImAxis_X1,
                            std::max(time_stream.value - history, 0.0f), std::max(history, time_stream.value),
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

  ImGui::End();
}
