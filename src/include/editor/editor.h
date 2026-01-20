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

#include "editor/project.h"
#include "editor/timeline.h"
#include "engine/engine.h"
#include "engine/openmpt_player.h"
#include "fx/mandelbrot_effect.h"

class Editor
{
public:
  Editor() = default;

  int run();

private:

  /// @brief Whether to add an imgui window with buttons to show debug toasts
  static constexpr bool kDebugToasts = false;

  enum class EditorStyle : uint8_t
  {
    PURPLE,
    WIN11DARK
  };

  static constexpr const char *const kChooseAudioDlgKey = "ChooseAudioDlgKey";
  static constexpr const char *const kLoadProjectDlgKey = "LoadProjectDlgKey";
  static constexpr const char *const kSaveProjectDlgKey = "SaveProjectDlgKey";

  static inline std::string format_time(double seconds)
  {
    const int minutes = static_cast<int>(seconds) / 60;
    const int secs = static_cast<int>(seconds) % 60;

    return std::to_string(minutes) + ":" + (secs < 10 ? "0" : "") +
           std::to_string(secs);
  }

  // TODO: use the same function and pass in EditorStyle as argument
  static void style_purple(ImGuiStyle &style);
  static void style_win11dark(ImGuiStyle &style);

  /// @brief Initializes the FX rendering system.
  void init_fx_system();

  /// @brief Initializes the current project.
  void init_project();

  void main_event_loop();

  /**
   * Renders a preview of the Mandelbrot effect, and allows the user
   * to modify the effect's parameters.
   *
   * The preview is rendered in a 400x300 pixel window, and is
   * updated when the user changes any of the effect's parameters.
   */
  void render_fx_preview();

  /// @brief Renders the main menu bar.
  void render_audio_tracks();
  void render_menu();
  void render_preview_image();
  void render_timeline();

  void display_dialogs();

  void open_audio_track_dialog();
  void open_load_project_dialog();
  void open_save_project_dialog();

  SDL_Window *window{nullptr};
  ImGuiIO *io{nullptr};

  Engine engine;
  Project current_project;

  EditorTimeline timeline;

  std::array<std::unique_ptr<IEffect>,1> effects;
  int selected_effect = 0;
  /// For ImGui Combobox
  std::array<const char *,1> effect_names = { "Mandelbrot" };

  GLuint fx_fbo{0};
  GLuint fx_texture{0};

  int fx_preview_width{800};
  int fx_preview_height{600};
  bool fx_preview_dirty{true};
};

#endif // NAG_EDITOR_H