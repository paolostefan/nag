#ifndef NAG_UI_WINDOW_H
#define NAG_UI_WINDOW_H

#include <atomic>
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
  virtual void display_dialogs() {
  }

  void set_status_message(std::string message) noexcept {
    status_message = std::move(message);
    status_message_time = static_cast<float>(ImGui::GetTime());
  }

  /// @brief Status Message (fade out after kStatusMessageDuration) ─────────────────
  void print_status_message();


  static constexpr ImVec4 kDangerButton{1.f, 0.3f, 0.3f, 1.f};

  /// Font size for the UI. Adjust as needed for different screen DPIs.
  static constexpr float kUIFontSize = 14.f;

  /// Duration to show status messages before fading out (in seconds).
  static constexpr float kStatusMessageDuration{3.f};

  std::string title{};

  /// @brief General purpose feedback message.
  /// Empty string means no message to show.
  std::string status_message;

  SDL_Window *window{nullptr};
  SDL_GLContext gl_context{};

  ImGuiIO *io{nullptr};

  /// Timestamp (ImGui time) when status_message was set.
  float status_message_time{0.f};

  // Initial window parameters
  int start_width{0};
  int start_height{0};

  std::atomic<bool> running{true};
};


#endif //NAG_UI_WINDOW_H
