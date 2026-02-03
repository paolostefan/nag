#include "editor/ui_window.h"

#include "fa-solid-900.h"
#include "IconsFontAwesome6.h"
#include "implot.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "GL/glew.h"
#include "spdlog/spdlog.h"

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
  bool running = true;

  SDL_Event event;

  // style_purple(ImGui::GetStyle());
  // style_win11dark(ImGui::GetStyle());

  do {
    // Event handling
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT ||
          (event.type == SDL_WINDOWEVENT &&
           event.window.event == SDL_WINDOWEVENT_CLOSE &&
           event.window.windowID == SDL_GetWindowID(window)))
        running = false;
    }

    if (!running) {
      break;
    }

    // New imgui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    const ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                                              ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);

    render_ui();

    // Render UI
    ImGui::Render();
    glViewport(0, 0, static_cast<int>(io->DisplaySize.x), static_cast<int>(io->DisplaySize.y));
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  } while (running);
}

bool UIWindow::initialize() {
  // OpenGL version
  const auto glsl_version = "#version 150";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

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

  // Load Font Awesome 6
  io->Fonts->AddFontDefault();
  static constexpr ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMaxAdvanceX = 13.0f; // Use if you want to make the icon monospaced
  icons_config.FontDataOwnedByAtlas = false; // We don't want ImGui to free the font data

  io->Fonts->AddFontFromMemoryTTF(
    font_awesome_6_free_solid_900_otf,
    font_awesome_6_free_solid_900_otf_len,
    13.0f,
    &icons_config,
    icons_ranges);
  // End of font loading stuff

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  return true;
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
