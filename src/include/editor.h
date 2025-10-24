#ifndef NAG_EDITOR_H
#define NAG_EDITOR_H

#include <string>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "spdlog/spdlog.h"

#include "engine.h"
#include "openmpt_player.h"
#include "editor.h"
#include "project.h"

class Editor
{
public:
  Editor() = default;

  static inline std::string format_time(double seconds)
  {
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;

    return std::to_string(minutes) + ":" + (secs < 10 ? "0" : "") +
           std::to_string(secs);
  }

  int run();
  void main_event_loop();

private:
  SDL_Window *window{nullptr};
  ImGuiIO *io{nullptr};
};

#endif // NAG_EDITOR_H