#include <string>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "spdlog/spdlog.h"

#include "engine.h"
#include "openmpt_player.h"

std::string format_time(double seconds)
{
  int minutes = static_cast<int>(seconds) / 60;
  int secs = static_cast<int>(seconds) % 60;

  return std::to_string(minutes) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
}

int main(int argc, char **argv)
{
  // Inizializzazione SDL con supporto OpenGL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
  {
    spdlog::critical("Error initializing SDL: {}", SDL_GetError());
    return -1;
  }

  // Versione OpenGL
  const char *glsl_version = "#version 150";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  // Creazione finestra
  SDL_Window *window = SDL_CreateWindow(
      "Demo Editor",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      1280, 720,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // V-Sync

  // Setup Dear ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  bool running = true;
  SDL_Event event;

  std::string lastLoadedFile;

  bool firstFrame = true;

  Engine engine;
  OpenMptPlayer player;

  while (running)
  {
    // Gestione eventi
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        running = false;
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
      {
        running = false;
      }
    }

    // New imgui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                                        ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);

    // Main UI window
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoDecoration);

    if (ImGui::Button("Load audio track"))
    {
      IGFD::FileDialogConfig config;
      config.path = ".";
      ImGuiFileDialog::Instance()->OpenDialog("ChooseAudioDlgKey", "Choose audio file", ".mod,.xm,.mp3", config);
    }

    // Audio track load dialog
    if (ImGuiFileDialog::Instance()->Display("ChooseAudioDlgKey"))
    {
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        lastLoadedFile = ImGuiFileDialog::Instance()->GetFilePathName();

        if (!player.load(lastLoadedFile))
        {
          spdlog::error("Error loading asset {}", lastLoadedFile);
        }
        else
        {
          player.play();
        }

        // if (!engine.LoadAsset(lastLoadedFile))
        // {
        //   spdlog::error("Error loading asset {}", lastLoadedFile);
        // }
        // else
        // {
        //   // Do stuff like playing the asset
        // }
      }

      ImGuiFileDialog::Instance()->Close();
    }

    const std::string audio_file = player.get_audio_path_str();
    if (!audio_file.empty())
    {
      ImGui::SameLine();
      ImGui::Text("Audio path: %s", audio_file.c_str());
      ImGui::Separator();

      const PlaybackState state = player.get_playback_state();

      if (state == PlaybackState::PLAYING)
      {
        if (ImGui::Button("Pause"))
        {
          player.pause();
        }
      }
      else
      {
        ImGui::SameLine();
        if (ImGui::Button("Play"))
        {
          player.play();
        }
      }

      ImGui::SameLine();
      if (ImGui::Button("Stop"))
      {
        player.stop();
      }

      const double current_ms = player.get_position_ms();
      const double duration_ms = player.get_duration_ms();

      const double current_seconds = current_ms / 1000.0;
      const double total_seconds = duration_ms / 1000.0;

      if (duration_ms > 0)
      {
        const float progress = static_cast<float>(current_ms / duration_ms);
        float slider_value = progress;
        if(ImGui::SliderFloat("Progress", &slider_value, 0.0f, 1.0f, "%.2f%%"))
        {
          double new_pos_ms = duration_ms * slider_value;
          spdlog::info("Seeking to {}", new_pos_ms);
          player.seek(new_pos_ms);
        }

        ImGui::Text("Time: %s / %s", format_time(current_seconds).c_str(), format_time(total_seconds).c_str());

        ImGui::Text("BPM: %.2f", player.get_bpm());
        ImGui::Text("Pattern/Row: %d/%02x", player.get_pattern(), player.get_row());
      }

      // State indicator
      const char *stateText = "";
      switch (state)
      {
      case PlaybackState::STOPPED:
        stateText = "Stopped";
        break;
      case PlaybackState::PLAYING:
        stateText = "Playing";
        break;
      case PlaybackState::PAUSED:
        stateText = "Paused";
        break;
      }
      ImGui::Text("Status: %s", stateText);
    }

    ImGui::End();

    // Render UI
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
