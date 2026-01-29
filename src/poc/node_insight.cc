#include "node_insight.h"

#include "IconsFontAwesome6.h"
#include "implot.h"

#include "engine/node.h"
#include "editor/scrolling_buffer.h"

int main(int argc, char **argv) {
  return NodeInsight().run();
}

void NodeInsight::render_ui()
{
  ImGui::Begin("Node Insight");

  static ScrollingBuffer buffer_in_a(1000);
  static ScrollingBuffer buffer_in_b(1000);
  static ScrollingBuffer buffer_out(1000);

  static SinStream sin_stream(3.111, 1.75);
  static SinStream cos_stream(1.0, 2.1, M_PI / 2.0);

  static Stream<float> out_stream;

  static AddFloatNode adding_node(&sin_stream, &cos_stream, &out_stream);

  static float t = 0.0f;
  static bool flow = true;
  static float history = 10.0f;

  if (!flow) {
    if (ImGui::Button(ICON_FA_PLAY "##play")) {
      flow = true;
    }
  }
  else if (ImGui::Button(ICON_FA_PAUSE "##pause")) {
    flow = false;
  }

  ImGui::SameLine();
  ImGui::Text("Time: %.2fs", t);
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    t = 0.0f;
    buffer_in_a.Erase();
    buffer_in_b.Erase();
    buffer_out.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &history, 1.0f, 30.0f);

  if (flow) {
    t += ImGui::GetIO().DeltaTime;
    buffer_in_a.AddPoint(t, sin_stream.at(t));
    buffer_in_b.AddPoint(t, cos_stream.at(t));

    adding_node.evaluate();

    buffer_out.AddPoint(t, out_stream.value);
  }

  static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;

  if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
    ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
    ImPlot::SetupAxisLimits(ImAxis_X1,
                            std::max(t - history, 0.0f), std::max(history, t),
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

