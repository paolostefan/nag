cmake_minimum_required(VERSION 3.20)
project(nag LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenGL is required by SDL2
find_package(OpenGL REQUIRED)

# GLEW is required by the ShaderManager
find_package(GLEW REQUIRED)

# ============================================================================
# DEPENDENCIES managed by FetchContent
# ============================================================================

# SDL2
include(FetchContent)
FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.32.10
)
FetchContent_MakeAvailable(SDL2)

# Imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/)

# Imguifiledialog
FetchContent_Declare(
  imguifiledialog
  GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
  GIT_TAG master
)
FetchContent_MakeAvailable(imguifiledialog)

add_library(imgui_fd STATIC
${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp  
)
target_link_libraries(imgui_fd PUBLIC imgui)
target_include_directories(imgui_fd PUBLIC ${imguifiledialog_SOURCE_DIR}/)

# nlohmann/json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG develop
)
FetchContent_MakeAvailable(nlohmann_json)

# OpenMPT
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENMPT REQUIRED libopenmpt)

if(NOT OPENMPT_FOUND)
  message(ERROR "OpenMPT not found")
endif()

# Spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v2.x
)

FetchContent_MakeAvailable(spdlog)

# IconFontCppHeaders
FetchContent_Declare(
  iconfontcppheaders
  GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
  GIT_TAG main
)

FetchContent_MakeAvailable(iconfontcppheaders)
Message(STATUS "iconfontcppheaders source dir: ${iconfontcppheaders_SOURCE_DIR}")

# ============================================================================
# END DEPENDENCIES
# ============================================================================

# Engine
file(GLOB ENGINE_SRCS src/engine/*.cc src/fx/*.cc)
add_library(nag_engine_lib ${ENGINE_SRCS})
target_include_directories(nag_engine_lib PUBLIC 
  src/include
  ${OPENMPT_INCLUDE_DIRS}
)

target_link_libraries(nag_engine_lib PUBLIC 
  OpenGL::GL
  GLEW::GLEW
  SDL2::SDL2
  mpg123
  spdlog
  ${OPENMPT_LIBRARIES}
  nlohmann_json::nlohmann_json
) 

target_compile_options(nag_engine_lib PRIVATE ${OPENMPT_CFLAGS_OTHER})

# Editor library
add_library(nag_editor_lib 
  src/editor/editor.cc
  src/editor/toast_manager.cc
  src/_3rd_party/ImGuizmo/ImSequencer.cpp
)

target_include_directories(nag_editor_lib PUBLIC
  src/_3rd_party/ImGuizmo
  ${iconfontcppheaders_SOURCE_DIR}/
)

target_link_libraries(nag_editor_lib PUBLIC
  nag_engine_lib
  imgui
  imgui_fd
)

# Editor
add_executable(editor src/editor/main.cc)
target_link_libraries(editor PRIVATE nag_editor_lib)

# Proofs of concept
add_executable(dummy_sdl_win src/poc/dummy_sdl_win.cc)
target_link_libraries(dummy_sdl_win PRIVATE SDL2::SDL2 SDL2::SDL2main)

# Copy shaders to build directory
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/src/shaders/*")
file(COPY ${SHADER_FILES} DESTINATION "${CMAKE_BINARY_DIR}/shaders")
message(STATUS "Shaders copied to ${CMAKE_BINARY_DIR}/shaders: ${SHADER_FILES}")

# Install shaders
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/shaders/" 
        DESTINATION "shaders"
        FILES_MATCHING PATTERN "*.vert" PATTERN "*.frag" PATTERN "*.comp")

# Enable testing
add_subdirectory(tests)