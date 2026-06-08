#include "editor/toast_manager.h"

#include <algorithm>

#include "IconsFontAwesome6.h"

void ToastManager::add_toast(ToastType type, const std::string &message, float duration)
{
  std::lock_guard<std::mutex> lock(toasts_mutex);

  toasts.emplace_back(type, message, duration);

  // Keep only the most recent toasts if we exceed the limit
  if (toasts.size() > max_visible_toasts * 2)
  {
    toasts.erase(toasts.begin(), toasts.begin() + (toasts.size() - max_visible_toasts));
  }
}

void ToastManager::render()
{
  std::lock_guard<std::mutex> lock(toasts_mutex);

  // Remove expired toasts
  toasts.erase(
      std::remove_if(toasts.begin(), toasts.end(),
                     [](const ToastNotification &toast)
                     { return toast.should_remove(); }),
      toasts.end());

  if (toasts.empty())
    return;

  ImGuiIO &io = ImGui::GetIO();

  // Calculate starting position (bottom-right corner)
  float y_pos = io.DisplaySize.y - position_offset_y;

  // Render toasts from bottom to top (newest at bottom)
  size_t visible_count = 0;
  for (auto it = toasts.rbegin(); it != toasts.rend() && visible_count < max_visible_toasts; ++it)
  {
    ToastNotification &toast = *it;
    toast.update_fade();

    if (toast.fade_progress <= 0.01f)
      continue;

    // Calculate position
    const float x_pos = io.DisplaySize.x - toast_width - position_offset_x;
    y_pos -= toast_height + toast_spacing;

    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(x_pos, y_pos), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(toast_width, toast_height), ImGuiCond_Always);

    // Get color based on toast type
    const ImVec4 bg_color = get_color_for_type(toast.type, 0.9f * toast.fade_progress);
    const ImVec4 border_color = get_color_for_type(toast.type, toast.fade_progress);

    // Apply alpha to text color
    const auto text_color = ImVec4(1.f, 1.f, 1.f, toast.fade_progress);

    // Push styles
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
    ImGui::PushStyleColor(ImGuiCol_Border, border_color);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 10.f));

    // Create window
    const std::string window_id = "##toast_" + std::to_string(reinterpret_cast<uintptr_t>(&toast));
    ImGui::Begin(window_id.c_str(),
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoFocusOnAppearing);

    // Icon and type label
    const char *icon = get_icon_for_type(toast.type);
    ImGui::TextUnformatted(icon);

    ImGui::SameLine();

    // Word-wrap the message
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + toast_width - 60.f);
    ImGui::TextWrapped("%s", toast.message.c_str());
    ImGui::PopTextWrapPos();

    // TODO Close button (only if hoverable - we need to remove NoInputs flag for this)
    // For now, toasts auto-dismiss only

    ImGui::End();

    // Pop styles
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);

    ++visible_count;
  }
}

void ToastManager::clear()
{
  std::lock_guard lock(toasts_mutex);
  toasts.clear();
}

ImVec4 ToastManager::get_color_for_type(const ToastType type, const float alpha)
{
  switch (type)
  {
  case ToastType::INFO:
    return {0.2f, 0.6f, 0.9f, alpha}; // Blue
  case ToastType::SUCCESS:
    return {0.2f, 0.8f, 0.4f, alpha}; // Green
  case ToastType::WARNING:
    return {0.9f, 0.7f, 0.2f, alpha}; // Orange
  case ToastType::ERROR:
    return {0.9f, 0.3f, 0.3f, alpha}; // Red
  default:
    return {0.5f, 0.5f, 0.5f, alpha}; // Gray
  }
}

const char *ToastManager::get_icon_for_type(const ToastType type)
{
  switch (type)
  {
  case ToastType::INFO:
    return ICON_FA_INFO;
  case ToastType::SUCCESS:
    return ICON_FA_CHECK;
  case ToastType::ERROR:
    return ICON_FA_BAN;
  case ToastType::WARNING:
  default:
    return ICON_FA_EXCLAMATION;
  }
}