#include "stream_insight.h"

#include "IconsFontAwesome6.h"
#include "imgui.h"
#include "implot.h"
#include "SDL.h"
#include "spdlog/spdlog.h"

#include "editor/scrolling_buffer.h"
#include "engine/stream.h"


int main(int argc, char **argv) {
  return StreamInsight().run();
}

void StreamInsight::render_ui() {
  ImGui::Begin("Stream Insight");

  static ScrollingBuffer buffer_sin(1000);
  static ScrollingBuffer buffer_cos(1000);

  static SinStream sin_stream;
  static SinStream cos_stream(1.0, 1.1, M_PI / 2.0);

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
    buffer_sin.Erase();
    buffer_cos.Erase();
  }

  ImGui::SameLine();
  ImGui::SliderFloat("History", &history, 1.0f, 30.0f);

  if (flow) {
    t += ImGui::GetIO().DeltaTime;
    buffer_sin.AddPoint(t, sin_stream.at(t));
    buffer_cos.AddPoint(t, cos_stream.at(t));
  }

  static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;

  if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
    ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
    ImPlot::SetupAxisLimits(ImAxis_X1,
                            std::max(t - history, 0.0f), std::max(history, t),
                            ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);

    if (!buffer_sin.Data.empty()) {
      ImPlot::PlotLine("Sin stream",
                       &buffer_sin.Data[0].x, &buffer_sin.Data[0].y,
                       buffer_sin.Data.size(),
                       0, buffer_sin.Offset, 2 * sizeof(float));
    }
    if (!buffer_cos.Data.empty()) {
      ImPlot::PlotLine("Cos stream",
                       &buffer_cos.Data[0].x, &buffer_cos.Data[0].y,
                       buffer_cos.Data.size(),
                       0, buffer_cos.Offset, 2 * sizeof(float));
    }

    ImPlot::EndPlot();
  }

  ImGui::End();
}
