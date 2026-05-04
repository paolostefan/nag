#ifndef NAG_UI_WINDOW_H
#define NAG_UI_WINDOW_H

#include <string>

#include "imgui.h"
#include "SDL.h"

/**
 * @brief Base class for an SDL window with an ImGui UI.
 *
 * Subclass and implement render_ui() to create a custom UI. The main event loop
 * and SDL/ImGui initialization are handled by this base class.
 */
class UIWindow {
public:
  virtual ~UIWindow() = default;

  UIWindow() = default;

  UIWindow(std::string title, int width, int height);

  int run();

protected:
  virtual void main_event_loop();

  virtual bool initialize();

  virtual void shutdown();

  virtual void render_ui() = 0;

  /// @brief Render any of the active ImGuiFileDialog instances.
  virtual void display_dialogs() {}

  static constexpr float kUIFontSize = 12.f;

  SDL_Window *window{nullptr};
  SDL_GLContext gl_context{};

  ImGuiIO *io{nullptr};

  std::string title{};
  int start_width{0};
  int start_height{0};
};


#endif //NAG_UI_WINDOW_H
