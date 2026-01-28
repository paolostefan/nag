#ifndef NAG_UI_WINDOW_H
#define NAG_UI_WINDOW_H

#include <string>

#include "imgui.h"
#include "SDL.h"

class UIWindow {

public:
  virtual ~UIWindow() = default;

  UIWindow() = default;

  UIWindow(std::string title, int width, int height);

  int run();

protected:
  virtual void main_event_loop();

  virtual void render_ui() = 0;

  SDL_Window *window{nullptr};
  ImGuiIO *io{nullptr};

  std::string title{};
  int start_width{0};
  int start_height{0};
};


#endif //NAG_UI_WINDOW_H