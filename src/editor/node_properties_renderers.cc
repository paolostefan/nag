#include "editor/node_properties_renderers.h"

#include <IconsFontAwesome6.h>
#include <cstring>

#include "imgui.h"

#include "editor/command_history.h"
#include "editor/property_widget.h"
#include "engine/node_graph.h"
#include "engine/node_type.h"
#include "engine/nodes/node.h"

#include "engine/nodes/circle_node.h"
#include "engine/nodes/clear_color_node.h"
#include "engine/nodes/color_correction_node.h"
#include "engine/nodes/composite_node.h"
#include "engine/nodes/displace_node.h"
#include "engine/nodes/effect_nodes.h"
#include "engine/nodes/ellipse_node.h"
#include "engine/nodes/generator_nodes.h"
#include "engine/nodes/gradient_node.h"
#include "engine/nodes/mandel_node.h"
#include "engine/nodes/math_nodes.h"
#include "engine/nodes/particle_emitter_node.h"
#include "engine/nodes/particle_renderer_node.h"
#include "engine/nodes/particle_system_node.h"
#include "engine/nodes/polygon_node.h"
#include "engine/nodes/rectangle2dnode.h"
#include "engine/nodes/sdf_shape_node.h"
#include "engine/nodes/temporal_nodes.h"
#include "engine/nodes/tile_node.h"
#include "engine/nodes/texture_loader_node.h"
#include "engine/nodes/transform_node.h"
#include "engine/nodes/vintage_crt_node.h"

void draw_node_properties(Node &node, NodeGraph &graph, CommandHistory &history) {
  switch (node.type) {
    case NodeType::Constant: {
      auto &n = dynamic_cast<ConstantFloatNode &>(node);
      PropertyWidget::SliderFloat(
        "Value", n.id, n.value,
        [](Node &nd, const float v) { dynamic_cast<ConstantFloatNode &>(nd).value = v; },
        graph, history, -20.f, 20.f, "%.3f");
      break;
    }

    case NodeType::Noise: {
      auto &n = dynamic_cast<NoiseNode &>(node);
      PropertyWidget::SliderFloat(
        "Frequency", n.id, n.frequency,
        [](Node &nd, const float v) { dynamic_cast<NoiseNode &>(nd).frequency = v; },
        graph, history, 0.1f, 10.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Amplitude", n.id, n.amplitude,
        [](Node &nd, const float v) { dynamic_cast<NoiseNode &>(nd).amplitude = v; },
        graph, history, 0.f, 2.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Persistence", n.id, n.persistence,
        [](Node &nd, const float v) { dynamic_cast<NoiseNode &>(nd).persistence = v; },
        graph, history, 0.f, 1.f, "%.2f");
      break;
    }

    case NodeType::Random: {
      auto &n = dynamic_cast<RandomNode &>(node);
      PropertyWidget::SliderFloat(
        "Min", n.id, n.min_value,
        [](Node &nd, const float v) {
          dynamic_cast<RandomNode &>(nd).set_range(v, dynamic_cast<RandomNode &>(nd).max_value);
        },
        graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Max", n.id, n.max_value,
        [](Node &nd, const float v) {
          dynamic_cast<RandomNode &>(nd).set_range(dynamic_cast<RandomNode &>(nd).min_value, v);
        },
        graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::InputInt(
        "Seed", n.id, n.seed,
        [](Node &nd, const int v) { dynamic_cast<RandomNode &>(nd).set_seed(v); },
        graph, history);
      break;
    }

    case NodeType::LFO: {
      auto &n = dynamic_cast<LFONode &>(node);
      PropertyWidget::SliderFloat(
        "Frequency", n.id, n.frequency,
        [](Node &nd, const float v) { dynamic_cast<LFONode &>(nd).frequency = v; },
        graph, history, 0.1f, 10.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Amplitude", n.id, n.amplitude,
        [](Node &nd, const float v) { dynamic_cast<LFONode &>(nd).amplitude = v; },
        graph, history, 0.f, 2.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Phase", n.id, n.phase,
        [](Node &nd, const float v) { dynamic_cast<LFONode &>(nd).phase = v; },
        graph, history, 0.f, 6.28f, "%.2f");
      PropertyWidget::SliderFloat(
        "Offset", n.id, n.offset,
        [](Node &nd, const float v) { dynamic_cast<LFONode &>(nd).offset = v; },
        graph, history, -1.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Pulse Width", n.id, n.pulse_width,
        [](Node &nd, const float v) { dynamic_cast<LFONode &>(nd).pulse_width = v; },
        graph, history, 0.01f, 0.99f, "%.2f");
      {
        const char *const wave_shapes[] = {"Sine", "Square", "Triangle", "Sawtooth"};
        PropertyWidget::Combo(
          "Wave Shape", n.id,
          reinterpret_cast<int &>(n.wave_shape),
          wave_shapes, 4,
          [](Node &nd, const int v) { dynamic_cast<LFONode &>(nd).wave_shape = static_cast<LFONode::WaveShape>(v); },
          graph, history);
      }
      break;
    }

    case NodeType::Envelope: {
      auto &n = dynamic_cast<EnvelopeNode &>(node);
      PropertyWidget::SliderFloat(
        "Attack", n.id, n.attack_time,
        [](Node &nd, const float v) { dynamic_cast<EnvelopeNode &>(nd).attack_time = v; },
        graph, history, 0.01f, 2.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Decay", n.id, n.decay_time,
        [](Node &nd, const float v) { dynamic_cast<EnvelopeNode &>(nd).decay_time = v; },
        graph, history, 0.01f, 2.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Sustain", n.id, n.sustain_level,
        [](Node &nd, const float v) { dynamic_cast<EnvelopeNode &>(nd).sustain_level = v; },
        graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Release", n.id, n.release_time,
        [](Node &nd, const float v) { dynamic_cast<EnvelopeNode &>(nd).release_time = v; },
        graph, history, 0.01f, 2.f, "%.2f");
      break;
    }

    case NodeType::Delay: {
      auto &n = dynamic_cast<DelayNode &>(node);
      PropertyWidget::SliderFloat(
        "Delay Time", n.id, n.delay_time,
        [](Node &nd, const float v) {
          auto &dn = dynamic_cast<DelayNode &>(nd);
          dn.delay_time = v;
          dn.update_buffer_size();
        },
        graph, history, 0.1f, 5.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Sample Rate", n.id, n.sample_rate,
        [](Node &nd, const float v) {
          auto &dn = dynamic_cast<DelayNode &>(nd);
          dn.sample_rate = v;
          dn.update_buffer_size();
        },
        graph, history, 10.f, 120.f, "%.0f");
      break;
    }

    case NodeType::Smoother: {
      auto &n = dynamic_cast<SmootherNode &>(node);
      PropertyWidget::SliderFloat(
        "Smooth Time", n.id, n.smooth_time,
        [](Node &nd, const float v) { dynamic_cast<SmootherNode &>(nd).smooth_time = v; },
        graph, history, 0.01f, 1.f, "%.2f");
      break;
    }

    case NodeType::Remap: {
      auto &n = dynamic_cast<RemapNode &>(node);
      PropertyWidget::SliderFloat(
        "In Min", n.id, n.in_min,
        [](Node &nd, const float v) { dynamic_cast<RemapNode &>(nd).in_min = v; },
        graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat(
        "In Max", n.id, n.in_max,
        [](Node &nd, const float v) { dynamic_cast<RemapNode &>(nd).in_max = v; },
        graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Out Min", n.id, n.out_min,
        [](Node &nd, const float v) { dynamic_cast<RemapNode &>(nd).out_min = v; },
        graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Out Max", n.id, n.out_max,
        [](Node &nd, const float v) { dynamic_cast<RemapNode &>(nd).out_max = v; },
        graph, history, 0.f, 1.f, "%.2f");
      break;
    }

    case NodeType::Clamp: {
      auto &n = dynamic_cast<ClampNode &>(node);
      PropertyWidget::SliderFloat(
        "Min", n.id, n.min_value,
        [](Node &nd, const float v) { dynamic_cast<ClampNode &>(nd).min_value = v; },
        graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat(
        "Max", n.id, n.max_value,
        [](Node &nd, const float v) { dynamic_cast<ClampNode &>(nd).max_value = v; },
        graph, history, 0.f, 1.f, "%.2f");
      break;
    }

    case NodeType::ClearColor: {
      auto &n = dynamic_cast<ClearColorNode &>(node);
      static ImVec4 im_color{n.color.x, n.color.y, n.color.z, n.color.w};
      PropertyWidget::ColorEdit4(
        "Color", n.id, im_color,
        [](Node &nd, const ImVec4 v) { dynamic_cast<ClearColorNode &>(nd).color = v; },
        graph, history);
      break;
    }

    case NodeType::Gradient: {
      auto &n = dynamic_cast<GradientNode &>(node);
      static constexpr const char *types[] = {"Linear", "Radial", nullptr};
      int gradient_type_int = static_cast<int>(n.gradient_type);
      PropertyWidget::Combo("Gradient type", n.id, gradient_type_int, types, 2,
                            [](Node &nd, int v) {
                              dynamic_cast<GradientNode &>(nd).gradient_type = static_cast<GradientNode::Type>(v);
                            },
                            graph, history);
      n.gradient_type = static_cast<GradientNode::Type>(gradient_type_int);

      auto *color_start_vec4 = reinterpret_cast<ImVec4 *>(&n.color_start);
      PropertyWidget::ColorEdit4("Start color", n.id, *color_start_vec4,
                                 [](Node &nd, const ImVec4 &col) {
                                   dynamic_cast<GradientNode &>(nd).color_start = col;
                                 },
                                 graph, history);
      auto *color_end_vec4 = reinterpret_cast<ImVec4 *>(&n.color_end);
      PropertyWidget::ColorEdit4("End color", n.id, *color_end_vec4,
                                 [](Node &nd, const ImVec4 &col) { dynamic_cast<GradientNode &>(nd).color_end = col; },
                                 graph, history);
      break;
    }

    case NodeType::Circle: {
      auto &n = dynamic_cast<CircleNode &>(node);
      auto *color_ = reinterpret_cast<ImVec4 *>(&n.color);
      PropertyWidget::ColorEdit4("Color", n.id, *color_,
                                 [](Node &nd, const ImVec4 &col) { dynamic_cast<CircleNode &>(nd).color = col; },
                                 graph, history);
      PropertyWidget::SliderFloat("Edge smoothness", n.id, n.edge_smoothness,
                                  [](Node &nd, const float v) { dynamic_cast<CircleNode &>(nd).edge_smoothness = v; },
                                  graph, history, 0.f, 1.f);
      break;
    }

    case NodeType::Ellipse: {
      auto &n = dynamic_cast<EllipseNode &>(node);
      auto *color_ = reinterpret_cast<ImVec4 *>(&n.color);
      PropertyWidget::ColorEdit4("Color", n.id, *color_,
                                 [](Node &nd, const ImVec4 &v) { dynamic_cast<EllipseNode &>(nd).color = v; },
                                 graph, history);
      PropertyWidget::SliderFloat("Edge smoothness", n.id, n.edge_smoothness,
                                  [](Node &nd, const float v) { dynamic_cast<EllipseNode &>(nd).edge_smoothness = v; },
                                  graph, history, 0.f, 1.f, "%.3f");
      break;
    }

    case NodeType::Rectangle2D: {
      auto &n = dynamic_cast<Rectangle2DNode &>(node);
      auto *color_ = reinterpret_cast<ImVec4 *>(&n.color);
      PropertyWidget::ColorEdit4("Color", n.id, *color_,
                                 [](Node &nd, const ImVec4 &col) { dynamic_cast<Rectangle2DNode &>(nd).color = col; },
                                 graph, history);
      PropertyWidget::SliderFloat("Corner radius", n.id, n.corner_radius,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<Rectangle2DNode &>(nd).corner_radius = v;
                                  },
                                  graph, history, 0.f, 1.f);
      break;
    }

    case NodeType::Polygon: {
      auto &n = dynamic_cast<PolygonNode &>(node);
      auto *color_ = reinterpret_cast<ImVec4 *>(&n.color);
      PropertyWidget::ColorEdit4("Color", n.id, *color_,
                                 [](Node &nd, const ImVec4 &v) { dynamic_cast<PolygonNode &>(nd).color = v; },
                                 graph, history);
      ImGui::SliderInt("Sides", &n.n_sides, 3, 12);
      PropertyWidget::SliderFloat("Edge smoothness", n.id, n.edge_smoothness,
                                  [](Node &nd, const float v) { dynamic_cast<PolygonNode &>(nd).edge_smoothness = v; },
                                  graph, history, 0.f, 1.f, "%.3f");
      break;
    }

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

    case NodeType::Tile: {
      auto &n = dynamic_cast<TileNode &>(node);
      PropertyWidget::DragFloat("Tile X", n.id, n.tile_x,
                                [](Node &nd, const float v) { dynamic_cast<TileNode &>(nd).tile_x = v; },
                                graph, history, 0.1f, 1.f, 32.f, "%.1f",
                                n.get_input("tile_x")->connected);
      PropertyWidget::DragFloat("Tile Y", n.id, n.tile_y,
                                [](Node &nd, const float v) { dynamic_cast<TileNode &>(nd).tile_y = v; },
                                graph, history, 0.1f, 1.f, 32.f, "%.1f",
                                n.get_input("tile_y")->connected);
      PropertyWidget::DragFloat("Offset X", n.id, n.offset_x,
                                [](Node &nd, const float v) { dynamic_cast<TileNode &>(nd).offset_x = v; },
                                graph, history, 0.005f, -1.f, 1.f, "%.3f",
                                n.get_input("offset_x")->connected);
      PropertyWidget::DragFloat("Offset Y", n.id, n.offset_y,
                                [](Node &nd, const float v) { dynamic_cast<TileNode &>(nd).offset_y = v; },
                                graph, history, 0.005f, -1.f, 1.f, "%.3f",
                                n.get_input("offset_y")->connected);
      ImGui::Checkbox("Mirror X", &n.mirror_x);
      ImGui::Checkbox("Mirror Y", &n.mirror_y);
      break;
    }

    case NodeType::ParticleRenderer: {
      auto &n = dynamic_cast<ParticleRendererNode &>(node);
      PropertyWidget::SliderFloat("Color Jitter", n.id, n.color_jitter,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).color_jitter = v;
                                  },
                                  graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat("Alpha Jitter", n.id, n.alpha_jitter,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).alpha_jitter = v;
                                  },
                                  graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat("Size Min", n.id, n.size_min,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).size_min = v;
                                  },
                                  graph, history, 0.5f, 50.f, "%.1f");
      PropertyWidget::SliderFloat("Size Max", n.id, n.size_max,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).size_max = v;
                                  },
                                  graph, history, 0.5f, 50.f, "%.1f");
      PropertyWidget::SliderFloat("Size Scatter", n.id, n.size_scatter,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).size_scatter = v;
                                  },
                                  graph, history, 0.f, 20.f, "%.1f");
      PropertyWidget::SliderFloat("Global Scale", n.id, n.global_scale,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).global_scale = v;
                                  },
                                  graph, history, 0.01f, 10.f, "%.2f");
      PropertyWidget::SliderFloat("Emitter X", n.id, n.emitter_x,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).emitter_x = v;
                                  },
                                  graph, history, 0.f, 1.f, "%.2f");
      PropertyWidget::SliderFloat("Emitter Y", n.id, n.emitter_y,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ParticleRendererNode &>(nd).emitter_y = v;
                                  },
                                  graph, history, 0.f, 1.f, "%.2f");
      break;
    }

    case NodeType::Blur: {
      auto &n = dynamic_cast<BlurNode &>(node);
      PropertyWidget::SliderFloat(
        "Radius", n.id, n.radius,
        [](Node &nd, const float v) { dynamic_cast<BlurNode &>(nd).radius = v; },
        graph, history, 0.f, 64.f, "%.1f",
        n.get_input("radius")->connected);
      break;
    }

    case NodeType::ChromaticAberration: {
      auto &n = dynamic_cast<ChromaticAberrationNode &>(node);
      PropertyWidget::SliderFloat("Strength", n.id, n.strength,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ChromaticAberrationNode &>(nd).strength = v;
                                  },
                                  graph, history, 0.f, 20.f, "%.1f");
      break;
    }

    case NodeType::ColorCorrection: {
      auto &n = dynamic_cast<ColorCorrectionNode &>(node);
      PropertyWidget::SliderFloat("Brightness", n.id, n.brightness,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ColorCorrectionNode &>(nd).brightness = v;
                                  },
                                  graph, history, -1.f, 1.f, "%.2f",
                                  n.get_input("brightness")->connected);
      PropertyWidget::SliderFloat("Contrast", n.id, n.contrast,
                                  [](Node &nd, const float v) { dynamic_cast<ColorCorrectionNode &>(nd).contrast = v; },
                                  graph, history, 0.f, 4.f, "%.2f",
                                  n.get_input("contrast")->connected);
      PropertyWidget::SliderFloat("Saturation", n.id, n.saturation,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ColorCorrectionNode &>(nd).saturation = v;
                                  },
                                  graph, history, 0.f, 2.f, "%.2f",
                                  n.get_input("saturation")->connected);
      PropertyWidget::SliderFloat("Hue shift", n.id, n.hue_shift,
                                  [](Node &nd, const float v) {
                                    dynamic_cast<ColorCorrectionNode &>(nd).hue_shift = v;
                                  },
                                  graph, history, 0.f, 6.2832f, "%.2f",
                                  n.get_input("hue_shift")->connected);
      break;
    }

    case NodeType::Composite: {
      auto &n = dynamic_cast<CompositeNode &>(node);
      static constexpr const char *modes[] = {"Normal", "Add", "Multiply", "Screen", nullptr};
      int blend_mode_int = static_cast<int>(n.blend_mode);
      PropertyWidget::Combo("Blend mode", n.id, blend_mode_int, modes, 4,
                            [](Node &nd, int value) {
                              dynamic_cast<CompositeNode &>(nd).blend_mode = static_cast<CompositeNode::BlendMode>(
                                value);
                            },
                            graph, history);
      n.blend_mode = static_cast<CompositeNode::BlendMode>(blend_mode_int);
      PropertyWidget::SliderFloat("Opacity", n.id, n.opacity,
                                  [](Node &nd, float value) { dynamic_cast<CompositeNode &>(nd).opacity = value; },
                                  graph, history, 0.f, 1.f);
      break;
    }

    case NodeType::Displace: {
      auto &n = dynamic_cast<DisplaceNode &>(node);
      PropertyWidget::SliderFloat("Strength", n.id, n.strength,
                                  [](Node &nd, const float v) { dynamic_cast<DisplaceNode &>(nd).strength = v; },
                                  graph, history, 0.f, 0.5f, "%.3f",
                                  n.get_input("strength")->connected);
      constexpr const char *kChannels[] = {"R", "G", "B"};
      ImGui::Combo("X channel", &n.channel_x, kChannels, 3);
      ImGui::Combo("Y channel", &n.channel_y, kChannels, 3);
      break;
    }

    case NodeType::Pixelate: {
      auto &n = dynamic_cast<PixelateNode &>(node);
      PropertyWidget::SliderFloat("Pixel Size", n.id, n.pixel_size,
                                  [](Node &nd, const float v) { dynamic_cast<PixelateNode &>(nd).pixel_size = v; },
                                  graph, history, 1.f, 64.f, "%.0f");
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
          0.01f, .0f, 2.f * PI,
          "%.04f",
          n.get_input("radius")->connected // disable if the corresponding pin is connected
        );
      }

      if (n.shape == SDFShapeNode::Shape::Ring) {
        ImGui::DragFloat("Ring thickness", &n.ring_thickness, 0.005f, 0.01f, 1.f, "%.3f");
      }

      auto *color_ = reinterpret_cast<ImVec4 *>(&n.color);
      PropertyWidget::ColorEdit4("Color", n.id, *color_,
                                 [](Node &nd, const ImVec4 &v) { dynamic_cast<SDFShapeNode &>(nd).color = v; },
                                 graph, history);
      PropertyWidget::SliderFloat("Edge smoothness", n.id, n.edge_smoothness,
                                  [](Node &nd, const float v) { dynamic_cast<SDFShapeNode &>(nd).edge_smoothness = v; },
                                  graph, history, 0.f, 1.f, "%.3f");
      break;
    }

    case NodeType::Transform: {
      auto &n = dynamic_cast<TransformNode &>(node);
      PropertyWidget::DragFloat("Translate X", n.id, n.translate_x,
                                [](Node &nd, const float v) { dynamic_cast<TransformNode &>(nd).translate_x = v; },
                                graph, history, 0.005f, -1.f, 1.f, "%.3f",
                                n.get_input("translate_x")->connected);
      PropertyWidget::DragFloat("Translate Y", n.id, n.translate_y,
                                [](Node &nd, const float v) { dynamic_cast<TransformNode &>(nd).translate_y = v; },
                                graph, history, 0.005f, -1.f, 1.f, "%.3f",
                                n.get_input("translate_y")->connected);
      PropertyWidget::DragFloat("Scale", n.id, n.scale,
                                [](Node &nd, const float v) { dynamic_cast<TransformNode &>(nd).scale = v; },
                                graph, history, 0.01f, 0.01f, 10.f, "%.3f",
                                n.get_input("scale")->connected);
      PropertyWidget::DragFloat("Rotation", n.id, n.rotation,
                                [](Node &nd, const float v) { dynamic_cast<TransformNode &>(nd).rotation = v; },
                                graph, history, 0.01f, -3.14159f, 3.14159f, "%.3f",
                                n.get_input("rotation")->connected);
      break;
    }

    case NodeType::VintageCRT: {
      auto &n = dynamic_cast<VintageCRTNode &>(node);
      PropertyWidget::SliderFloat("Pixel Size", n.id, n.pixel_size,
                                  [](Node &nd, const float v) { dynamic_cast<VintageCRTNode &>(nd).pixel_size = v; },
                                  graph, history, 1.f, 64.f, "%.0f");
      break;
    }

    case NodeType::Mandel: {
      auto &n = dynamic_cast<MandelNode &>(node);
      PropertyWidget::DragFloat("Center X", n.id, n.center.x,
                                [](Node &nd, const float v) { dynamic_cast<MandelNode &>(nd).center.x = v; },
                                graph, history, 0.00001f, -3.f, 3.f, "%.06f",
                                n.get_input("center_x")->connected // disable if the corresponding pin is connected
      );

      PropertyWidget::DragFloat("Center Y", n.id, n.center.y,
                                [](Node &nd, const float v) { dynamic_cast<MandelNode &>(nd).center.y = v; },
                                graph, history, 0.00001f, -3.f, 3.f, "%.06f",
                                n.get_input("center_y")->connected // disable if the corresponding pin is connected
      );

      PropertyWidget::SliderFloat("Zoom", n.id, n.zoom,
                                  [](Node &nd, const float v) { dynamic_cast<MandelNode &>(nd).zoom = v; },
                                  graph, history, 0.05f, 10000.f, "%.3f",
                                  n.get_input("zoom")->connected, // disable if the corresponding pin is connected
                                  ImGuiSliderFlags_Logarithmic);

      PropertyWidget::DragInt("Iterations", n.id, n.iterations,
                              [](Node &nd, const int v) { dynamic_cast<MandelNode &>(nd).iterations = v; },
                              graph, history, 1, 10, 10000, "%d",
                              n.get_input("iterations")->connected // disable if the corresponding pin is connected
      );
      break;
    }

    case NodeType::ParticleEmitter: {
      auto &n = dynamic_cast<ParticleEmitterNode &>(node);
      PropertyWidget::SliderFloat("Rate", n.id, n.rate,
                                  [](Node &nd, const float v) { dynamic_cast<ParticleEmitterNode &>(nd).rate = v; },
                                  graph, history, 0.1f, 500.f, "%.1f");
      PropertyWidget::SliderFloat("Speed", n.id, n.speed,
                                  [](Node &nd, const float v) { dynamic_cast<ParticleEmitterNode &>(nd).speed = v; },
                                  graph, history, 0.f, 1000.f, "%.1f");
      PropertyWidget::SliderFloat("Min Life", n.id, n.min_life,
                                  [](Node &nd, const float v) { dynamic_cast<ParticleEmitterNode &>(nd).min_life = v; },
                                  graph, history, 0.1f, 10.f, "%.2f");
      PropertyWidget::SliderFloat("Max Life", n.id, n.max_life,
                                  [](Node &nd, const float v) { dynamic_cast<ParticleEmitterNode &>(nd).max_life = v; },
                                  graph, history, 0.1f, 10.f, "%.2f");
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
