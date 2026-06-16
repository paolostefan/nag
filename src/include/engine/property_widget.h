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
  // InputInt
  // ---------------------------------------------------------------------------

  /// @brief Renders an InputInt widget and emits an undo command on release.
  ///
  /// @param label       Widget label (also used as param_name in the command).
  /// @param node_id     ID of the owning node.
  /// @param value       Reference to the int field on the node.
  /// @param setter      Callable to restore the value during undo/redo.
  /// @param graph       The node graph (passed to history.execute).
  /// @param history     The command history.
  /// @param min         Minimum value (default 0).
  /// @param max         Maximum value (default 100).
  /// @param disabled    If true, the widget is rendered disabled and does not emit commands (default false).
  void InputInt(const std::string &label, int node_id, int &value,
                std::function<void(Node &, int)> setter,
                NodeGraph &graph, CommandHistory &history,
                int min = 0,
                int max = 100,
                bool disabled = false);

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
  /// @param min         Minimum value (default 0.f).
  /// @param max         Maximum value (default 0.f = no limit).
  /// @param format      Printf format string (default "%.3f").
  /// @param disabled    If true, the widget is rendered disabled and does not emit commands (default false).
  void DragFloat(const std::string &label, int node_id, float &value,
                 std::function<void(Node &, float)> setter,
                 NodeGraph &graph, CommandHistory &history,
                 float speed = 0.01f,
                 float min = 0.f,
                 float max = 0.f,
                 const char *format = "%.3f",
                 bool disabled = false);

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
  /// @param min     Minimum value (default 0.f).
  /// @param max     Maximum value (default 1.f).
  /// @param format  Printf format string (default "%.3f").
  /// @param disabled If true, the slider is rendered disabled and does not emit commands (default false).
  void SliderFloat(const std::string &label,
                   int node_id,
                   float &value,
                   std::function<void(Node &, float)> setter,
                   NodeGraph &graph,
                   CommandHistory &history,
                   float min = 0.f,
                   float max = 1.f,
                   const char *format = "%.3f",
                   bool disabled = false);

  // ---------------------------------------------------------------------------
  // ColorEdit4
  // ---------------------------------------------------------------------------

  /// @brief Renders a ColorEdit4 widget and emits an undo command on release.
  ///
  /// @param label    The label for the color edit widget.
  /// @param node_id  The ID of the node this property belongs to.
  /// @param value    Reference to an ImVec4 color field on the node.
  /// @param setter   Callable to restore the value during undo/redo.
  /// @param graph    The node graph (passed to history.execute).
  /// @param history  The command history.
  /// @param flags    ImGui color edit flags (default 0).
  /// @param disabled If true, the widget is rendered disabled and does not emit commands (default false).
  void ColorEdit4(const std::string &label, int node_id, ImVec4 &value,
                         std::function<void(Node &, ImVec4)> setter,
                         NodeGraph &graph, CommandHistory &history,
                         ImGuiColorEditFlags flags = 0,
                         bool disabled = false);

  // ---------------------------------------------------------------------------
  // Combo (for enums)
  // ---------------------------------------------------------------------------

  /// @brief Renders a Combo widget for enum-like int values.
  ///
  /// @param label      The label for the combo widget.
  /// @param node_id    The ID of the node this property belongs to.
  /// @param value      Reference to the int/enum field on the node.
  /// @param items      Null-terminated array of item labels (e.g. {"Normal", "Add",
  ///                  "Multiply", nullptr}).
  /// @param item_count The number of items in @ref items.
  /// @param setter     Callable accepting (Node&, int) to restore the value during
  ///                   undo/redo.
  /// @param graph      The node graph (passed to `history.execute()`).
  /// @param history    The command history.
  /// @param disabled   If true, the combo is rendered disabled and does not emit commands (default false).
  void Combo(const std::string &label, int node_id, int &value,
                    const char *const*items,
                    int item_count,
                    std::function<void(Node &, int)> setter,
                    NodeGraph &graph,
                    CommandHistory &history,
                    bool disabled = false);
} // namespace PropertyWidget

#endif // NAG_EDITOR_PROPERTY_WIDGET_H
