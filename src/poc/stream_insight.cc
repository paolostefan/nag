#include "SDL.h"
#include "spdlog/spdlog.h"
#include "imgui.h"
#include "implot.h"

#include "stream_insight.h"

#include "engine/stream.h"


int main(int argc, char **argv) {
  // Init SDL with OpenGL support
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
    spdlog::critical("Error initializing SDL: {}", SDL_GetError());
    return -1;
  }

  return StreamInsight().run();
}

void StreamInsight::render_ui() {
  ImGui::Begin("Stream Insight");

  static ScrollingBuffer buffer_x(1000);

  static SinStream sin_stream;

  static float t = 0.0f;
  t += ImGui::GetIO().DeltaTime;

  buffer_x.AddPoint(t, sin_stream.at(t));

  static float history = 10.0f;
  static ImPlotAxisFlags axis_flags = ImPlotAxisFlags_AutoFit;

  if (ImPlot::BeginPlot("##plot", ImVec2(-1, 150))) {
    ImPlot::SetupAxes(nullptr, nullptr, axis_flags, axis_flags);
    ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);

    ImPlot::PlotLine("Mouse X",
                     &buffer_x.Data[0].x, &buffer_x.Data[0].y,
                     buffer_x.Data.size(),
                     0, buffer_x.Offset, 2 * sizeof(float));

    ImPlot::EndPlot();
  }

  ImGui::End();
}
