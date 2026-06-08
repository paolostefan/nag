#ifndef NAG_EDITOR_TOAST_MANAGER_H
#define NAG_EDITOR_TOAST_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include <chrono>

#include "imgui.h"

enum class ToastType : uint8_t {
  INFO,
  SUCCESS,
  WARNING,
  ERROR
};

struct ToastNotification {
  ToastType type;
  std::string message;
  std::chrono::steady_clock::time_point creation_time;
  float duration_seconds;
  float fade_progress; // 0.0 = invisible, 1.0 = fully visible
  bool dismissed;

  ToastNotification(ToastType t, const std::string &msg, float duration)
    : type(t),
      message(msg),
      creation_time(std::chrono::steady_clock::now()),
      duration_seconds(duration),
      fade_progress(0.f),
      dismissed(false) {
  }

  // Get elapsed time in seconds
  float get_elapsed_seconds() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - creation_time);
    return elapsed.count() / 1000.f;
  }

  // Check if toast should be removed
  bool should_remove() const {
    return dismissed || (get_elapsed_seconds() > duration_seconds + 0.5f);
  }

  // Update fade progress based on elapsed time
  void update_fade() {
    float elapsed = get_elapsed_seconds();
    const float fade_in_duration = 0.3f;
    const float fade_out_duration = 0.5f;

    if (dismissed) {
      // Fast fade out when manually dismissed
      fade_progress -= 0.1f;
      if (fade_progress < 0.f)
        fade_progress = 0.f;
    } else if (elapsed < fade_in_duration) {
      // Fade in
      fade_progress = elapsed / fade_in_duration;
    } else if (elapsed > duration_seconds - fade_out_duration) {
      // Fade out
      float fade_out_elapsed = elapsed - (duration_seconds - fade_out_duration);
      fade_progress = 1.f - (fade_out_elapsed / fade_out_duration);
      if (fade_progress < 0.f)
        fade_progress = 0.f;
    } else {
      // Fully visible
      fade_progress = 1.f;
    }
  }
};

class ToastManager {
public:
  // Disable copy and move
  ToastManager(const ToastManager &) = delete;

  ToastManager &operator=(const ToastManager &) = delete;

  static ToastManager &instance() {
    static ToastManager inst;
    return inst;
  }

  // Add a new toast notification
  void add_toast(ToastType type, const std::string &message, float duration = 4.f);

  // Render all active toasts (call this in your main loop)
  void render();

  // Clear all toasts
  void clear();

  // Configuration
  void set_position_offset(float x, float y) {
    position_offset_x = x;
    position_offset_y = y;
  }

  void set_toast_width(float width) { toast_width = width; }
  void set_max_visible_toasts(size_t max) { max_visible_toasts = max; }

private:
  ToastManager() = default;

  ~ToastManager() = default;

  static ImVec4 get_color_for_type(ToastType type, float alpha = 1.f);

  static const char *get_icon_for_type(ToastType type);

  std::vector<ToastNotification> toasts;
  mutable std::mutex toasts_mutex;

  // Configuration
  float position_offset_x = 10.f;
  float position_offset_y = 10.f;
  float toast_width = 300.f;
  float toast_height = 80.f;
  float toast_spacing = 10.f;
  size_t max_visible_toasts = 5;
};

#endif // NAG_EDITOR_TOAST_MANAGER_H
