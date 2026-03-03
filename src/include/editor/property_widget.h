#ifndef NAG_EDITOR_PROPERTY_WIDGET_H
#define NAG_EDITOR_PROPERTY_WIDGET_H

#include <functional>
#include <string>
#include <unordered_map>

#include "imgui.h"
#include "editor/command_history.h"
#include "editor/set_node_param_command.h"
#include "engine/node_graph.h"

/// @file property_widget.h
/// @brief ImGui widget helpers for the Node Properties panel.
///
/// Each helper encapsulates the three-phase undo pattern:
///   1. IsItemActivated   → capture value_before into the before-map
///   2. During drag       → write directly to the parameter (real-time preview)
///   3. IsItemDeactivatedAfterEdit → emit SetNodeParamCommand to history
///
/// The before-map is keyed on "<node_id>:<param_name>" so multiple widgets
/// from different nodes can be active without collision.

namespace PropertyWidget {
  namespace internal {
    /// Returns a stable key for the before-value map.
    inline std::string MakeKey(const int node_id, const std::string &param_name) {
      return std::to_string(node_id) + ":" + param_name;
    }

    /// Per-type storage for "value before edit started".
    template<typename T>
    std::unordered_map<std::string, T> &BeforeMap() {
      static std::unordered_map<std::string, T> map;
      return map;
    }
  } // namespace internal

  // ---------------------------------------------------------------------------
  // DragFloat
  // ---------------------------------------------------------------------------

  /// @brief Renders a DragFloat widget and emits an undo command on release.
  ///
  /// @param label       Widget label (also used as param_name in the command).
  /// @param node_id     ID of the owning node.
  /// @param value       Reference to the float field on the node.
  /// @param setter      Callable to restore the value during undo/redo.
  /// @param graph       The node graph (passed to history.execute).
  /// @param history     The command history.
  /// @param speed       DragFloat speed (default 0.01f).
  /// @param min         Minimum value (default 0.0f).
  /// @param max         Maximum value (default 0.0f = no limit).
  /// @param format      Printf format string (default "%.3f").
  inline void DragFloat(const std::string &label, int node_id, float &value,
                        std::function<void(Node &, float)> setter,
                        NodeGraph &graph, CommandHistory &history,
                        const float speed = 0.01f, const float min = 0.0f, const float max = 0.0f,
                        const char *format = "%.3f") {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<float>();

    ImGui::DragFloat(label.c_str(), &value, speed, min, max, format);

    if (ImGui::IsItemActivated()) {
      before_map[key] = value;
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      auto it = before_map.find(key);
      if (it != before_map.end()) {
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
  }

  // ---------------------------------------------------------------------------
  // SliderFloat
  // ---------------------------------------------------------------------------

  /// @brief Renders a SliderFloat widget and emits an undo command on release.
  ///
  /// @param label   The label for the slider widget.
  /// @param node_id The ID of the node this property belongs to.
  /// @param value   Reference to a float field on the node.
  /// @param setter  Callable to restore the value during undo/redo.
  /// @param graph   The node graph (passed to history.execute).
  /// @param history The command history.
  /// @param min     Minimum value (default 0.0f).
  /// @param max     Maximum value (default 1.0f).
  /// @param format  Printf format string (default "%.3f").
  inline void SliderFloat(const std::string &label, int node_id, float &value,
                          std::function<void(Node &, float)> setter,
                          NodeGraph &graph, CommandHistory &history,
                          const float min = 0.0f, const float max = 1.0f,
                          const char *format = "%.3f") {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<float>();

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
  }

  // ---------------------------------------------------------------------------
  // ColorEdit4
  // ---------------------------------------------------------------------------

  /// @brief Renders a ColorEdit4 widget and emits an undo command on release.
  ///
  /// @param label   The label for the color edit widget.
  /// @param node_id The ID of the node this property belongs to.
  /// @param value   Reference to an ImVec4 color field on the node.
  /// @param setter  Callable to restore the value during undo/redo.
  /// @param graph   The node graph (passed to history.execute).
  /// @param history The command history.
  /// @param flags   ImGui color edit flags (default 0).
  inline void ColorEdit4(const std::string &label, int node_id, ImVec4 &value,
                         std::function<void(Node &, ImVec4)> setter,
                         NodeGraph &graph, CommandHistory &history,
                         const ImGuiColorEditFlags flags = 0) {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<ImVec4>();

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
  }

  // ---------------------------------------------------------------------------
  // Combo (for enums)
  // ---------------------------------------------------------------------------

  /// @brief Renders a Combo widget for enum-like int values.
  ///
  /// @param label The label for the combo widget.
  /// @param node_id The ID of the node this property belongs to.
  /// @param value Reference to the int/enum field on the node.
  /// @param items Null-terminated array of item labels (e.g. {"Normal", "Add",
  ///              "Multiply", nullptr}).
  /// @param item_count The number of items in @ref items.
  /// @param setter Callable accepting (Node&, int) to restore the value during
  ///                undo/redo.
  /// @param graph The node graph (passed to `history.execute()`).
  /// @param history The command history.
  inline void Combo(const std::string &label, int node_id, int &value,
                    const char *const*items,
                    int item_count,
                    std::function<void(Node &, int)> setter,
                    NodeGraph &graph, CommandHistory &history) {
    const std::string key = internal::MakeKey(node_id, label);
    auto &before_map = internal::BeforeMap<int>();

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
  }
} // namespace PropertyWidget

#endif // NAG_EDITOR_PROPERTY_WIDGET_H
