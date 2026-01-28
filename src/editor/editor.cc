#include "editor/editor.h"

#include "IconsFontAwesome6.h"

#include "editor/toast_manager.h"


void Editor::init_fx_system() {
  effects[0] = std::make_unique<MandelbrotEffect>();
  if (!effects[0]->initialize()) {
    spdlog::error("Error initializing mandelbrot effect");
    effects[0].reset();
    return;
  }

  // Create framebuffer for FX preview rendering
  glGenFramebuffers(1, &fx_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fx_fbo);

  // Create texture for FX preview rendering
  glGenTextures(1, &fx_texture);
  glBindTexture(GL_TEXTURE_2D, fx_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
               fx_preview_width, fx_preview_height,
               0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, fx_texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("FX framebuffer not complete");
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Editor::init_project() {
  // todo reload last project from ...?
}

void Editor::render_preview_image() const {
  glBindFramebuffer(GL_FRAMEBUFFER, fx_fbo);
  glViewport(0, 0, fx_preview_width, fx_preview_height);

  // Clear color buffer
  glClearColor(.0f, .0f, .0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  double current_time = (double) current_project.current_frame /
                        (double) current_project.fps;
  effects[selected_effect]->render(fx_preview_width, fx_preview_height, current_time);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Editor::render_fx_preview() {
  ImGui::Begin("FX Preview");

  ImGui::Combo("Effect", &selected_effect,
               effect_names.data(), static_cast<int>(effect_names.size()));

  if (selected_effect >= 0 && selected_effect < effects.size() &&
      ImGui::CollapsingHeader(effect_names[selected_effect], ImGuiTreeNodeFlags_DefaultOpen)) {
    auto &params = effects[selected_effect]->get_parameters();
    for (auto &param: params) {
      const std::string &name = param.get_name();
      const std::string &fmt = param.get_fmt();
      const ParameterType type = param.get_type();

      switch (type) {
        case ParameterType::FLOAT: {
          float val = std::get<float>(param.get_value());

          const float min_val = std::get<float>(param.get_min_value());
          const float max_val = std::get<float>(param.get_max_value());
          const float step = std::get<float>(param.get_step());

          if (ImGui::DragFloat(name.c_str(), &val,
                               step, min_val, max_val,
                               fmt == "" ? "%.3f" : fmt.c_str())) {
            param.set_value(val);
            fx_preview_dirty = true;
          }
          break;
        }
        case ParameterType::INT: {
          int val = std::get<int>(param.get_value());

          const int min_val = std::get<int>(param.get_min_value());
          const int max_val = std::get<int>(param.get_max_value());
          const int step = std::get<int>(param.get_step());

          if (ImGui::DragInt(name.c_str(), &val, step, min_val, max_val,
                             fmt == "" ? "%d" : fmt.c_str())) {
            param.set_value(val);
            fx_preview_dirty = true;
          }
          break;
        }
        case ParameterType::VEC2: {
          Vec2 val = std::get<Vec2>(param.get_value());

          const Vec2 min_val = std::get<Vec2>(param.get_min_value());
          const Vec2 max_val = std::get<Vec2>(param.get_max_value());
          const Vec2 step = std::get<Vec2>(param.get_step());

          bool changed = false;

          changed |= ImGui::DragFloat((name + ".x").c_str(), &val.x,
                                      step.x, min_val.x, max_val.x,
                                      fmt == "" ? "%.3f" : fmt.c_str());
          changed |= ImGui::DragFloat((name + ".y").c_str(), &val.y,
                                      step.y, min_val.y, max_val.y,
                                      fmt == "" ? "%.3f" : fmt.c_str());

          if (changed) {
            param.set_value(val);
            fx_preview_dirty = true;
          }
          break;
        }
        default:
          spdlog::error("Unknown parameter type for {}", name);
          break;
      }
    }

    // Render effect preview
    if (fx_preview_dirty) {
      render_preview_image();
    }

    // Get available width
    const float avail_width = ImGui::GetContentRegionAvail().x;

    // Draw effect Image to the max space available
    ImGui::Image((void *) (intptr_t) fx_texture,
                 ImVec2(avail_width, fx_preview_height * (avail_width / fx_preview_width)),
                 ImVec2(0, 1), ImVec2(1, 0));

    // ImGui::Image((void *)(intptr_t)fx_texture,
    //              ImVec2(400, 300),
    //              ImVec2(0, 1), ImVec2(1, 0));

    if (ImGui::Button(ICON_FA_PLUS " Add to timeline")) {
      current_project.timeline.Add(TrackType::EFFECT);
      current_project.pristine = false;

      TimelineTrack &new_track = current_project.timeline.tracks.back();
      new_track.frameStart = current_project.current_frame;
      new_track.frameEnd = current_project.timeline.GetFrameMax();
    }
  }

  ImGui::End();
}

void Editor::render_menu() {
  ImGui::BeginMainMenuBar();

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open project...")) {
      open_load_project_dialog();
    }

    if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save project as...")) {
      open_save_project_dialog();
    }

    if (ImGui::MenuItem(ICON_FA_VOLUME_HIGH " Load audio track")) {
      open_audio_track_dialog();
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ICON_FA_RIGHT_FROM_BRACKET " Exit")) {
      SDL_Event quit_event;
      quit_event.type = SDL_QUIT;
      SDL_PushEvent(&quit_event);
    }

    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void Editor::render_audio_tracks() {
  ImGui::Begin("Audio Tracks");

  if (ImGui::Button("Load audio track")) {
    open_audio_track_dialog();
  }

  uint8_t track_no = 0;

  for (const AudioTrack &track: current_project.audio_tracks) {
    ImGui::TextUnformatted(track.title.c_str());
    ImGui::SameLine();

    IAudioPlayer &player = *track.player;
    const PlaybackState state = player.get_playback_state();

    const std::string track_no_s = std::to_string(track_no);

    if (state == PlaybackState::PLAYING) {
      if (ImGui::Button((ICON_FA_PAUSE "##" + track_no_s).c_str())) {
        player.pause();
      }
    } else {
      if (ImGui::Button((ICON_FA_PLAY "##" + track_no_s).c_str())) {
        player.play();
      }
    }

    ImGui::SameLine();
    if (ImGui::Button((ICON_FA_STOP "##" + track_no_s).c_str())) {
      player.stop();
    }

    const double current_ms = player.get_position_ms();
    const double duration_ms = player.get_duration_ms();

    const double current_seconds = current_ms / 1000.0;
    const double total_seconds = duration_ms / 1000.0;

    if (duration_ms > 0.05) {
      ImGui::SameLine();
      const auto progress = static_cast<float>(current_ms * 100.0 / duration_ms);
      float slider_value = progress;

      if (ImGui::SliderFloat(("##TrackProg_" + track_no_s).c_str(),
                             &slider_value, 0.0f, 100.0f, "%.2f%%")) {
        const double new_pos_ms = duration_ms * slider_value / 100.0;
        if (std::abs(new_pos_ms - current_ms) > 20.0) {
          player.seek(new_pos_ms);
        }
        // spdlog::debug("Seeking to {}ms (slider {})", new_pos_ms, slider_value);
      }

      ImGui::SameLine();
      ImGui::Text("%s / %s", format_time(current_seconds).c_str(), format_time(total_seconds).c_str());

      if (track.track_is_module) {
        ImGui::SameLine();
        ImGui::Text("%.1f BPM, %d/%02x", player.get_bpm(), player.get_pattern(), player.get_row());
      }
    }

    ++track_no;
  }

  ImGui::End();
}

void Editor::render_timeline() {
  static bool expanded = true;
  static int selected_entry = -1;
  static int first_frame = 0;

  ImGui::Begin("Timeline");

  ImSequencer::Sequencer(
    &current_project.timeline,
    &current_project.current_frame,
    &expanded,
    &selected_entry,
    &first_frame,
    ImSequencer::SEQUENCER_EDIT_ALL |
    ImSequencer::SEQUENCER_ADD |
    ImSequencer::SEQUENCER_DEL |
    ImSequencer::SEQUENCER_COPYPASTE |
    ImSequencer::SEQUENCER_CHANGE_FRAME);

  ImGui::End();
}

void Editor::display_dialogs() {
  // Audio track load dialog
  if (ImGuiFileDialog::Instance()->Display(kChooseAudioDlgKey)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const std::string audio_file_path = ImGuiFileDialog::Instance()->GetFilePathName();

      try {
        current_project.add_audio_track(audio_file_path);
        ToastManager::instance().add_toast(ToastType::SUCCESS, "Audio track loaded successfully");
      } catch (const std::exception &e) {
        spdlog::error("Error loading audio track '{}': {}",
                      audio_file_path, e.what());
      }
    }

    ImGuiFileDialog::Instance()->Close();
  }

  if (ImGuiFileDialog::Instance()->Display(kLoadProjectDlgKey)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const std::string project_path = ImGuiFileDialog::Instance()->GetFilePathName();

      if (load_project(project_path)) {
        ToastManager::instance().add_toast(ToastType::SUCCESS, "Project loaded successfully");
      } else {
        ToastManager::instance().add_toast(ToastType::ERROR, "Error loading project");
      }
    }

    ImGuiFileDialog::Instance()->Close();
  }

  if (ImGuiFileDialog::Instance()->Display(kSaveProjectDlgKey)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const std::string project_save_path = ImGuiFileDialog::Instance()->GetFilePathName();

      save_project(project_save_path);
    }

    ImGuiFileDialog::Instance()->Close();
  }
}

void Editor::open_audio_track_dialog() const {
  static IGFD::FileDialogConfig config;

  if (current_project.audio_tracks.empty())
    config.path = "~";
  else {
    const auto &last_track = current_project.audio_tracks.back();
    config.path = last_track.path.parent_path().string();
  }

  ImGuiFileDialog::Instance()->OpenDialog(kChooseAudioDlgKey, "Choose audio file", ".mod,.xm,.mp3", config);
}

void Editor::open_load_project_dialog() const {
  static IGFD::FileDialogConfig config;

  if (current_project.path.empty())
    config.path = "~";
  else
    config.path = current_project.path.string();

  ImGuiFileDialog::Instance()->OpenDialog(kLoadProjectDlgKey, "Load Project", ".nagproj", config);
}

void Editor::open_save_project_dialog() const {
  static IGFD::FileDialogConfig config;

  // Open the save file dialog
  if (current_project.path.empty())
    config.path = "~";
  else
    config.path = current_project.path.string();

  ImGuiFileDialog::Instance()->OpenDialog(kSaveProjectDlgKey, "Save Project", ".nagproj", config);
}

void Editor::main_event_loop() {
  init_fx_system();
  init_project();

  UIWindow::main_event_loop();
}

void Editor::render_ui() {
  render_menu();

  if constexpr (kDebugToasts) {
    ImGui::Begin("Debug Toasts");
    if (ImGui::Button("Info Toast")) {
      ToastManager::instance().add_toast(ToastType::INFO, "This is an info toast");
    }

    ImGui::SameLine();

    if (ImGui::Button("Success Toast")) {
      ToastManager::instance().add_toast(ToastType::SUCCESS, "This is a success toast");
    }

    ImGui::SameLine();

    if (ImGui::Button("Warning Toast")) {
      ToastManager::instance().add_toast(ToastType::WARNING, "This is a warning toast");
    }

    ImGui::SameLine();

    if (ImGui::Button("Error Toast")) {
      ToastManager::instance().add_toast(ToastType::ERROR, "This is an error toast");
    }
    ImGui::End();
  }

  ImGui::Begin("Project Info", nullptr, ImGuiWindowFlags_NoDecoration);
  ImGui::TextUnformatted(current_project.name.c_str());
  ImGui::End();

  render_timeline();
  render_fx_preview();
  render_audio_tracks();

  ToastManager::instance().render();

  display_dialogs();
}

void Editor::style_purple(ImGuiStyle &style) {
  // Purple Comfy style by RegularLunar from ImThemes
  style.Alpha = 1.0f;
  style.DisabledAlpha = 0.1f;
  style.WindowPadding = ImVec2(8.0f, 8.0f);
  style.WindowRounding = 10.0f;
  style.WindowBorderSize = 0.0f;
  style.WindowMinSize = ImVec2(30.0f, 30.0f);
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_Right;
  style.ChildRounding = 5.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupRounding = 10.0f;
  style.PopupBorderSize = 0.0f;
  style.FramePadding = ImVec2(5.0f, 3.5f);
  style.FrameRounding = 5.0f;
  style.FrameBorderSize = 0.0f;
  style.ItemSpacing = ImVec2(5.0f, 4.0f);
  style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
  style.CellPadding = ImVec2(4.0f, 2.0f);
  style.IndentSpacing = 5.0f;
  style.ColumnsMinSpacing = 5.0f;
  style.ScrollbarSize = 15.0f;
  style.ScrollbarRounding = 9.0f;
  style.GrabMinSize = 15.0f;
  style.GrabRounding = 5.0f;
  style.TabRounding = 5.0f;
  style.TabBorderSize = 0.0f;
  style.TabCloseButtonMinWidthSelected = 0.0f;
  style.TabCloseButtonMinWidthUnselected = 0.0f;
  style.ColorButtonPosition = ImGuiDir_Right;
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

  style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, 0.360515f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
  style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38039216f, 0.42352942f, 0.57254905f, 0.54901963f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.25882354f, 0.25882354f, 0.25882354f, 0.0f);
  style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 0.0f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.23529412f, 0.23529412f, 0.23529412f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.29411766f, 0.29411766f, 0.29411766f, 1.0f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_Separator] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_Tab] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_TabHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_TabActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.4509804f, 1.0f, 0.0f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 0.0f);
  style.Colors[ImGuiCol_PlotLines] = ImVec4(0.29411766f, 0.29411766f, 0.29411766f, 1.0f);
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
  style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
  style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.2901961f);
  style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03433478f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
  style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
  style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
  style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

void Editor::style_win11dark(ImGuiStyle &style) {
  // Windark style by DestroyerDarkNess from ImThemes
  style.Alpha = 1.0f;
  style.DisabledAlpha = 0.6f;
  style.WindowPadding = ImVec2(8.0f, 8.0f);
  style.WindowRounding = 8.4f;
  style.WindowBorderSize = 1.0f;
  style.WindowMinSize = ImVec2(32.0f, 32.0f);
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_Right;
  style.ChildRounding = 3.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupRounding = 3.0f;
  style.PopupBorderSize = 1.0f;
  style.FramePadding = ImVec2(4.0f, 3.0f);
  style.FrameRounding = 3.0f;
  style.FrameBorderSize = 1.0f;
  style.ItemSpacing = ImVec2(8.0f, 4.0f);
  style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
  style.CellPadding = ImVec2(4.0f, 2.0f);
  style.IndentSpacing = 21.0f;
  style.ColumnsMinSpacing = 6.0f;
  style.ScrollbarSize = 5.6f;
  style.ScrollbarRounding = 18.0f;
  style.GrabMinSize = 10.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 3.0f;
  style.TabBorderSize = 0.0f;
  style.TabCloseButtonMinWidthSelected = 0.0f;
  style.TabCloseButtonMinWidthUnselected = 0.0f;
  style.ColorButtonPosition = ImGuiDir_Right;
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

  style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
  style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
  style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.34901962f, 0.34901962f, 0.34901962f, 1.0f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.32941177f, 0.6f, 1.0f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
  style.Colors[ImGuiCol_Separator] = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
  style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
  style.Colors[ImGuiCol_Tab] = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
  style.Colors[ImGuiCol_TabHovered] = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
  style.Colors[ImGuiCol_TabActive] = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
  style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.0f, 0.32941177f, 0.6f, 1.0f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.0f, 0.32941177f, 0.6f, 1.0f);
  style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
  style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
  style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
  style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
  style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
  style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 1.0f);
  style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
  style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

bool Editor::save_project(const std::string &save_path) noexcept {
  if (current_project.pristine) {
    spdlog::error("Cowardly refusing to save an unmodified project");
    return false;
  }

  spdlog::info("Saving project to '{}'...", save_path);
  current_project.path = save_path;

  bool result = false;

  try {
    const nlohmann::json j(current_project);
    std::ofstream save_file(save_path);
    save_file << std::setw(2) << j << std::endl;

    current_project.pristine = true;
    result = true;

    spdlog::info("Project saved successfully");
  } catch (const std::exception &e) {
    spdlog::error("Failed to save project: {}", e.what());
  }

  return result;
}

bool Editor::load_project(const std::string &load_path) noexcept {
  current_project.path = load_path;

  bool result = false; // Assume failure
  try {
    std::ifstream load_file(load_path);
    nlohmann::json j;
    load_file >> j;

    current_project = j.get<Project>();

    result = true;
  } catch (const std::exception &e) {
    spdlog::error("Failed to load project: {}", e.what());
  }

  return result;
}
