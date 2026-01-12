#ifndef NAG_EDITOR_H
#define NAG_EDITOR_H

#include <string>

#include "GL/glew.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "spdlog/spdlog.h"

#include "engine/engine.h"
#include "fx/mandelbrot_effect.h"
#include "openmpt_player.h"
#include "project.h"

class Editor
{
public:
  Editor() = default;

  int run();

private:
  enum class EditorStyle : uint8_t
  {
    PURPLE,
    WIN11DARK
  };

  static constexpr const char *const kChooseAudioDlgKey = "ChooseAudioDlgKey";
  static constexpr const char *const kSaveProjectDlgKey = "SaveProjectDlgKey";

  static inline std::string format_time(double seconds)
  {
    const int minutes = static_cast<int>(seconds) / 60;
    const int secs = static_cast<int>(seconds) % 60;

    return std::to_string(minutes) + ":" + (secs < 10 ? "0" : "") +
           std::to_string(secs);
  }

  static void style_purple(ImGuiStyle &style);
  static void style_win11dark(ImGuiStyle &style);

  void init_fx_system();

  void main_event_loop();

  void render_fx_preview();
  void render_menu();
  void render_preview_image();
  void render_audio_tracks();
  
  void display_dialogs();

  void open_audio_track_dialog();
  void open_save_project_dialog();

  SDL_Window *window{nullptr};
  ImGuiIO *io{nullptr};

  Engine engine;
  OpenMptPlayer player;
  Project current_project;

  std::unique_ptr<MandelbrotEffect> mandel_effect;
  GLuint fx_fbo{0};
  GLuint fx_texture{0};

  int fx_preview_width{800};
  int fx_preview_height{600};
  bool fx_preview_dirty{true};
};

#endif // NAG_EDITOR_H