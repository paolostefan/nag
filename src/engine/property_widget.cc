#include "engine/property_widget.h"

namespace PropertyWidget {
  void DragFloat(const std::string &label,
                        int node_id,
                        float &value,
                        std::function<void(Node &, float)> setter,
                        NodeGraph &graph,
                        CommandHistory &history,
                        const float speed,
                        const float min,
                        const float max,
                        const char *format,
                        const bool disabled) {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<float>();

    ImGui::BeginDisabled(disabled);
    ImGui::DragFloat(label.c_str(), &value, speed, min, max, format);

    if (ImGui::IsItemActivated()) {
      before_map[key] = value;
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      if (auto it = before_map.find(key); it != before_map.end()) {
        const float value_before = it->second;
        before_map.erase(it);

        // Only emit a command if the value actually changed.
        if (value_before != value) {
          history.execute(
            graph,
            std::make_unique<SetNodeParamCommand<float> >(
              node_id, label, value_before, value, std::move(setter)));
        }
      }
    }

    ImGui::EndDisabled();
  }

  void SliderFloat(const std::string &label,
                          int node_id,
                          float &value,
                          std::function<void(Node &, float)> setter,
                          NodeGraph &graph,
                          CommandHistory &history,
                          const float min,
                          const float max,
                          const char *format,
                          const bool disabled) {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<float>();

    ImGui::BeginDisabled(disabled);
    ImGui::SliderFloat(label.c_str(), &value, min, max, format);

    if (ImGui::IsItemActivated()) {
      before_map[key] = value;
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      auto it = before_map.find(key);
      if (it != before_map.end()) {
        const float value_before = it->second;
        before_map.erase(it);

        if (value_before != value) {
          history.execute(
            graph,
            std::make_unique<SetNodeParamCommand<float> >(
              node_id, label, value_before, value, std::move(setter)));
        }
      }
    }

    ImGui::EndDisabled();
  }

  void ColorEdit4(const std::string &label,
                         int node_id,
                         ImVec4 &value,
                         std::function<void(Node &, ImVec4)> setter,
                         NodeGraph &graph,
                         CommandHistory &history,
                         const ImGuiColorEditFlags flags,
                         const bool disabled) {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<ImVec4>();

    ImGui::BeginDisabled(disabled);

    ImGui::ColorEdit4(label.c_str(), reinterpret_cast<float *>(&value), flags);

    if (ImGui::IsItemActivated()) {
      before_map[key] = value;
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      const auto it = before_map.find(key);
      if (it != before_map.end()) {
        const ImVec4 value_before = it->second;
        before_map.erase(it);

        // ImVec4 has no operator==; compare component-wise.
        const bool changed =
            value_before.x != value.x || value_before.y != value.y ||
            value_before.z != value.z || value_before.w != value.w;

        if (changed) {
          history.execute(
            graph,
            std::make_unique<SetNodeParamCommand<ImVec4> >(
              node_id, label, value_before, value, std::move(setter)));
        }
      }
    }

    ImGui::EndDisabled();
  }

  void Combo(const std::string &label, int node_id, int &value,
                    const char *const*items,
                    const int item_count,
                    std::function<void(Node &, int)> setter,
                    NodeGraph &graph,
                    CommandHistory &history,
                    const bool disabled) {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<int>();

    ImGui::BeginDisabled(disabled);

    // Capture before on open (Activated fires on first click on a Combo).
    if (ImGui::IsItemActivated()) {
      before_map[key] = value;
    }

    if (ImGui::Combo(label.c_str(), &value, items, item_count)) {
      // Combo changes value immediately on selection — capture before if not
      // already done, then emit command right away (no drag phase).
      const auto it = before_map.find(key);
      const int value_before = it != before_map.end() ? it->second : value;
      before_map.erase(key);

      if (value_before != value) {
        history.execute(
          graph,
          std::make_unique<SetNodeParamCommand<int> >(
            node_id, label, value_before, value, std::move(setter)));
      }
    }

    ImGui::EndDisabled();
  }
} // namespace PropertyWidget
