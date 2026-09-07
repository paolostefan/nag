#include "editor/node_properties_renderers.h"

#include <IconsFontAwesome6.h>
#include <cstring>

#include "imgui.h"

#include "editor/command_history.h"
#include "editor/property_widget.h"
#include "engine/node_graph.h"
#include "engine/node_type.h"
#include "engine/nodes/node.h"

#include "engine/nodes/mandel_node.h"
#include "engine/nodes/particle_system_node.h"
#include "engine/nodes/sdf_shape_node.h"
#include "engine/nodes/texture_loader_node.h"

namespace {

Vec4 to_vec4(const ImVec4 &v) {
  return {v.x, v.y, v.z, v.w};
}

// ============================================================================
// Schema-driven property rendering
//
// Iterates node.properties() and dispatches on WidgetKind, mapping each row to
// the matching PropertyWidget helper (which preserves the 3-phase undo path).
// Rows whose value lives on a pin that is already connected are disabled.
// ============================================================================

bool draw_schema_properties(Node &node, NodeGraph &graph, CommandHistory &history) {
  const auto &props = node.properties();
  if (props.empty()) {
    return false;
  }

  for (const auto &p : props) {
    bool disabled = false;
    if (!p.disable_pin.empty()) {
      if (const Pin *pin = node.get_input(p.disable_pin)) {
        disabled = pin->connected;
      }
    }

    const auto setter = [p](Node &, PropertyValue v) { p.set(v); };

    switch (p.kind) {
      case WidgetKind::SliderFloat: {
        if (auto *ptr = std::get_if<float *>(&p.value)) {
          ImGuiSliderFlags flags = 0;
          if (p.slider_flags == SliderFlag::Logarithmic) {
            flags |= ImGuiSliderFlags_Logarithmic;
          }
          PropertyWidget::SliderFloat(p.label, node.id, **ptr,
                                      [setter](Node &nd, float v) { setter(nd, v); },
                                      graph, history, p.min, p.max,
                                      p.format ? p.format : "%.3f",
                                      disabled, flags);
        }
        break;
      }

      case WidgetKind::DragFloat: {
        if (auto *ptr = std::get_if<float *>(&p.value)) {
          PropertyWidget::DragFloat(p.label, node.id, **ptr,
                                    [setter](Node &nd, float v) { setter(nd, v); },
                                    graph, history, p.speed, p.min, p.max,
                                    p.format ? p.format : "%.3f",
                                    disabled);
        }
        break;
      }

      case WidgetKind::SliderInt: {
        if (auto *ptr = std::get_if<int *>(&p.value)) {
          PropertyWidget::SliderInt(p.label, node.id, **ptr,
                                    [setter](Node &nd, int v) { setter(nd, v); },
                                    graph, history,
                                    static_cast<int>(p.min), static_cast<int>(p.max),
                                    p.format ? p.format : "%d",
                                    disabled);
        }
        break;
      }

      case WidgetKind::DragInt: {
        if (auto *ptr = std::get_if<int *>(&p.value)) {
          PropertyWidget::DragInt(p.label, node.id, **ptr,
                                  [setter](Node &nd, int v) { setter(nd, v); },
                                  graph, history, p.speed,
                                  static_cast<int>(p.min), static_cast<int>(p.max),
                                  p.format ? p.format : "%d",
                                  disabled);
        }
        break;
      }

      case WidgetKind::InputInt: {
        if (auto *ptr = std::get_if<int *>(&p.value)) {
          PropertyWidget::InputInt(p.label, node.id, **ptr,
                                   [setter](Node &nd, int v) { setter(nd, v); },
                                   graph, history,
                                   static_cast<int>(p.min), static_cast<int>(p.max),
                                   disabled);
        }
        break;
      }

      case WidgetKind::Combo: {
        if (auto *ptr = std::get_if<int *>(&p.value)) {
          PropertyWidget::Combo(p.label, node.id, **ptr,
                                p.items, p.item_count,
                                [setter](Node &nd, int v) { setter(nd, v); },
                                graph, history, disabled);
        }
        break;
      }

      case WidgetKind::Checkbox: {
        if (auto *ptr = std::get_if<bool *>(&p.value)) {
          PropertyWidget::Checkbox(p.label, node.id, **ptr,
                                   [setter](Node &nd, bool v) { setter(nd, v); },
                                   graph, history, disabled);
        }
        break;
      }

      case WidgetKind::ColorEdit: {
        if (auto *ptr = std::get_if<Vec4 *>(&p.value)) {
          auto &im_color = reinterpret_cast<ImVec4 &>(**ptr);
          PropertyWidget::ColorEdit4(p.label, node.id, im_color,
                                     [setter](Node &nd, const ImVec4 &v) { setter(nd, to_vec4(v)); },
                                     graph, history);
        }
        break;
      }
    }
  }
  return true;
}

}  // namespace

void draw_node_properties(Node &node, NodeGraph &graph, CommandHistory &history) {
  if (draw_schema_properties(node, graph, history)) {
    return;
  }

  switch (node.type) {
    case NodeType::TextureLoader: {
      auto &n = dynamic_cast<TextureLoaderNode &>(node);
      if (n.file_path.empty()) {
        ImGui::TextDisabled("No file loaded.");
      } else {
        const std::string filename = n.file_path.substr(n.file_path.find_last_of("/\\") + 1);
        ImGui::TextUnformatted(filename.c_str());
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", n.file_path.c_str());
        }
      }
      ImGui::Spacing();
      if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Browse...")) {
        n.open_file_dialog();
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_ROTATE_RIGHT "  Reload")) {
        n.set_path(n.file_path);
      }
      break;
    }

    case NodeType::SDFShape: {
      auto &n = dynamic_cast<SDFShapeNode &>(node);
      static constexpr const char *kShapes[] = {"Circle", "Box", "Ring"};
      int shape_idx = static_cast<int>(n.shape);
      if (ImGui::Combo("Shape", &shape_idx, kShapes, 3)) {
        n.shape = static_cast<SDFShapeNode::Shape>(shape_idx);
      }

      PropertyWidget::DragFloat(
        "Position X", n.id, n.position.x,
        [](Node &nd, const float v) {
          dynamic_cast<SDFShapeNode &>(nd).position.x = v;
        },
        graph, history,
        0.0001f, -1.f, 1.f,
        "%.04f",
        n.get_input("pos_x")->connected // disable if the corresponding pin is connected
      );

      PropertyWidget::DragFloat(
        "Position Y", n.id, n.position.y,
        [](Node &nd, const float v) {
          dynamic_cast<SDFShapeNode &>(nd).position.y = v;
        },
        graph, history,
        0.0001f, -1.f, 1.f,
        "%.04f",
        n.get_input("pos_y")->connected // disable if the corresponding pin is connected
      );

      PropertyWidget::DragFloat(
        "Radius", n.id, n.radius,
        [](Node &nd, const float v) {
          dynamic_cast<SDFShapeNode &>(nd).radius = v;
        },
        graph, history,
        0.0001f, .0001f, 2.f,
        "%.04f",
        n.get_input("radius")->connected // disable if the corresponding pin is connected
      );

      if (n.shape == SDFShapeNode::Shape::Box) {
        PropertyWidget::DragFloat(
          "Aspect ratio", n.id, n.aspect,
          [](Node &nd, const float v) {
            dynamic_cast<SDFShapeNode &>(nd).aspect = v;
          },
          graph, history,
          0.01f, .1f, 10.f,
          "%.2f"
        );
        PropertyWidget::DragFloat(
          "Rotation", n.id, n.rotation,
          [](Node &nd, const float v) {
            dynamic_cast<SDFShapeNode &>(nd).rotation = v;
          },
          graph, history,
          0.01f, .0f, 2.f * 3.14159265358979f,
          "%.04f",
          n.get_input("radius")->connected // disable if the corresponding pin is connected
        );
      }

      if (n.shape == SDFShapeNode::Shape::Ring) {
        ImGui::DragFloat("Ring thickness", &n.ring_thickness, 0.005f, 0.01f, 1.f, "%.3f");
      }

      auto *color_ = reinterpret_cast<ImVec4 *>(&n.color);
      PropertyWidget::ColorEdit4("Color", n.id, *color_,
                                 [](Node &nd, const ImVec4 &v) { dynamic_cast<SDFShapeNode &>(nd).color = to_vec4(v); },
                                 graph, history);
      PropertyWidget::SliderFloat("Edge smoothness", n.id, n.edge_smoothness,
                                  [](Node &nd, const float v) { dynamic_cast<SDFShapeNode &>(nd).edge_smoothness = v; },
                                  graph, history, 0.f, 1.f, "%.3f");
      break;
    }

    case NodeType::Mandel: {
      auto &n = dynamic_cast<MandelNode &>(node);

      // Center is a composite Vec2 ([x,y] serialized as one array) → per-node hook.
      PropertyWidget::DragFloat("Center X", n.id, n.center.x,
                                [](Node &nd, const float v) { dynamic_cast<MandelNode &>(nd).center.x = v; },
                                graph, history, 0.00001f, -3.f, 3.f, "%.06f",
                                n.get_input("center_x")->connected /*
                                  disable if the corresponding pin is connected
                                  */
      );

      PropertyWidget::DragFloat("Center Y", n.id, n.center.y,
                                [](Node &nd, const float v) { dynamic_cast<MandelNode &>(nd).center.y = v; },
                                graph, history, 0.00001f, -3.f, 3.f, "%.06f",
                                n.get_input("center_y")->connected /*
                                  disable if the corresponding pin is connected
                                  */
      );

      PropertyWidget::SliderFloat("Zoom", n.id, n.zoom,
                                  [](Node &nd, const float v) { dynamic_cast<MandelNode &>(nd).zoom = v; },
                                  graph, history, 0.05f, 10000.f, "%.3f",
                                  n.get_input("zoom")->connected,
                                  ImGuiSliderFlags_Logarithmic);

      PropertyWidget::DragInt("Iterations", n.id, n.iterations,
                              [](Node &nd, const int v) { dynamic_cast<MandelNode &>(nd).iterations = v; },
                              graph, history, 1, 10, 10000, "%d",
                              n.get_input("iterations")->connected);
      break;
    }

    case NodeType::ParticleSystem: {
      auto &n = dynamic_cast<ParticleSystemNode &>(node);
      static constexpr const char *kForceTypes[] = {
        "Acceleration X", "Acceleration Y", "Radial", "Drag X", "Drag Y", "Vortex",
      };

      if (n.inputs.size() <= 1) {
        ImGui::TextDisabled("No input forces.");
      }

      if (ImGui::Button(ICON_FA_PLUS "  Add Force")) {
        Pin *force_pin = n.add_input(DataType::Float, "force " + std::to_string(n.inputs.size()));
        force_pin->id = graph.pin_id_generator.generate_id();
        force_pin->pad = static_cast<uint8_t>(ParticleSystemNode::ParticleSystemForce::AccelerationX);
      }

      for (size_t i = 1; i < n.inputs.size(); ++i) {
        const char *const pin_name = n.inputs[i].name.c_str();
        ImGui::PushID(n.inputs[i].id);
        ImGui::TextUnformatted(pin_name);
        ImGui::SameLine();
        ImGui::BeginDisabled(n.is_renaming);
        int force_type = n.inputs[i].pad - static_cast<uint8_t>(ParticleSystemNode::ParticleSystemForce::AccelerationX);
        if (ImGui::Combo("Force type", &force_type, kForceTypes, 6)) {
          n.inputs[i].pad = static_cast<uint8_t>(
            static_cast<int>(ParticleSystemNode::ParticleSystemForce::AccelerationX) + force_type);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_I_CURSOR)) {
          n.is_renaming = true;
          n.rename_pin_id = n.inputs[i].id;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Rename this force pin (%s)", pin_name);
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.f));
        if (ImGui::SmallButton(ICON_FA_XMARK)) {
          n.remove_input(i);
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Remove this force pin (%s)", pin_name);
        }
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::PopID();
      }

      const std::string popup_id = "RenamePinPopup" + std::to_string(n.id);
      if (n.is_renaming) {
        ImGui::OpenPopup(popup_id.c_str());
      }
      if (ImGui::BeginPopup(popup_id.c_str())) {
        Pin *force_pin = n.get_input_by_id(n.rename_pin_id);
        if (ImGui::IsWindowAppearing()) {
          if (!force_pin) {
            ImGui::TextColored(ImVec4(200, 50, 50, 255), "Pin %d not found!", n.rename_pin_id);
          } else {
            ImGui::SetKeyboardFocusHere();
            strncpy(n.pin_name_buf, force_pin->name.c_str(), sizeof(n.pin_name_buf));
          }
        }
        if (force_pin) {
          ImGui::InputText("##renamePinNewName", n.pin_name_buf, sizeof(n.pin_name_buf));
          ImGui::SameLine();
          ImGui::BeginDisabled(strlen(n.pin_name_buf) == 0);
          if (ImGui::Button("OK")) {
            force_pin->name = n.pin_name_buf;
            ImGui::CloseCurrentPopup();
            n.is_renaming = false;
          }
          ImGui::EndDisabled();
        }
        ImGui::EndPopup();
      }
      break;
    }

    // Nodes without properties
    case NodeType::Default:
    case NodeType::Time:
    case NodeType::Output:
    case NodeType::StepSequencer:
    case NodeType::Abs:
    case NodeType::Floor:
    case NodeType::Ceil:
    case NodeType::Round:
    case NodeType::Sqrt:
    case NodeType::Negate:
    case NodeType::Sin:
    case NodeType::Cos:
    case NodeType::Tan:
    case NodeType::Subtract:
    case NodeType::Multiply:
    case NodeType::Divide:
    case NodeType::Modulo:
    case NodeType::Power:
    case NodeType::Compare:
    case NodeType::Add:
    case NodeType::Min:
    case NodeType::Max:
    case NodeType::Lerp:
    case NodeType::SmoothStep:
      break;

    case NodeType::Count:
      break;
  }
}