#include "editor/ui_window.h"

#include "misc/freetype/imgui_freetype.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "IconsFontAwesome6.h"
#include "implot.h"
#include "GL/glew.h"
#include "spdlog/spdlog.h"

#include "font/fa-solid-900.h"
#include "font/roboto-regular.h"

UIWindow::UIWindow(std::string title, const int width, const int height)
  : title(std::move(title)), start_width(width), start_height(height) {
  if (!UIWindow::initialize())
    throw std::runtime_error("Failed to initialize UI window");
}

int UIWindow::run() {
  main_event_loop();

  shutdown();
  return 0;
}

void UIWindow::main_event_loop() {

  SDL_Event event;

  while (running.load(std::memory_order_acquire)) {
    // Event handling
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT ||
          (event.type == SDL_WINDOWEVENT &&
           event.window.event == SDL_WINDOWEVENT_CLOSE &&
           event.window.windowID == SDL_GetWindowID(window)))
        running.store(false, std::memory_order_release);
    }

    if (!running.load(std::memory_order_acquire)) {
      break;
    }

    // New imgui frame
    SDL_GL_MakeCurrent(window, gl_context);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    const ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(
      0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);

    render_ui();

    // Render UI
    ImGui::Render();
    glViewport(0, 0,
               static_cast<int>(io->DisplaySize.x),
               static_cast<int>(io->DisplaySize.y));
    glClearColor(.1f, .1f, .1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
}

bool UIWindow::initialize() {
  // SDL must be initialized before creating the OpenGL context, which is required for GLEW initialization.
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
    spdlog::critical("SDL_Init failed: {}", SDL_GetError());
    return false;
  }

  // OpenGL version
  const auto glsl_version = "#version 330";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Main window
  window = SDL_CreateWindow(
    title.c_str(),
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    start_width, start_height,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // V-Sync

  // IMPORTANT: Initialize GLEW AFTER creating the OpenGL context
  glewExperimental = GL_TRUE; // Needed for core profile
  if (const GLenum glew_err = glewInit(); glew_err != GLEW_OK) {
    spdlog::critical("GLEW initialization failed: {}",
                     reinterpret_cast<const char *>(glewGetErrorString(glew_err)));
    return false;
  }

  spdlog::info("OpenGL version: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
  spdlog::info("GLSL version: {}", reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

  // Setup Dear ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // Setup ImPlot
  ImPlot::CreateContext();

  io = &ImGui::GetIO();
  io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Load default font
  ImFontConfig default_config;
  // These are for pixel-perfect fonts like Hooge
  default_config.OversampleH = default_config.OversampleV = 1;
  default_config.PixelSnapH = default_config.PixelSnapV = true;
  default_config.FontDataOwnedByAtlas = false; // We don't want ImGui to free the font data
  default_config.FontLoaderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_LoadColor;

  io->Fonts->AddFontFromMemoryTTF(Roboto_Regular_ttf, (int) Roboto_Regular_ttf_len,
                                  kUIFontSize, &default_config);

  // Load Font Awesome 6
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMinAdvanceX = kUIFontSize; // make icons monospaced to align properly in menus, toolbars, etc.
  icons_config.GlyphMaxAdvanceX = kUIFontSize;
  icons_config.FontDataOwnedByAtlas = false; // We don't want ImGui to free the font data

  static constexpr ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

  io->Fonts->AddFontFromMemoryTTF(
    font_awesome_6_free_solid_900_otf,
    font_awesome_6_free_solid_900_otf_len,
    kUIFontSize,
    &icons_config,
    icons_ranges);
  // End of font loading stuff

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  return true;
}

void UIWindow::print_status_message() {

  if (!status_message.empty()) {
    if (const float elapsed = static_cast<float>(ImGui::GetTime()) - status_message_time;
      elapsed < kStatusMessageDuration) {
      // Alpha: 100% for up to 1s from the end, then 1s fade out
      constexpr float fade_start = kStatusMessageDuration - 1.f;
      const float alpha = elapsed > fade_start
                            ? 1.f - (elapsed - fade_start)
                            : 1.f;

      ImGui::SameLine(0.f, 30.f);
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImVec4(0.6f, 1.f, 0.6f, alpha));
      ImGui::TextUnformatted(status_message.c_str());
      ImGui::PopStyleColor();
    } else {
      // Clear message after duration
      status_message.clear();
    }
  }
}

void UIWindow::shutdown() {
  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();

  ImPlot::DestroyContext();

  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
