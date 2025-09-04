cmake_minimum_required(VERSION 3.20)
project(nag LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

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
  GIT_TAG v0.6.7
)
FetchContent_MakeAvailable(imguifiledialog)

add_library(imgui_fd STATIC
  ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp  
)
target_link_libraries(imgui_fd PUBLIC imgui)
target_include_directories(imgui_fd PUBLIC ${imguifiledialog_SOURCE_DIR}/)

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

# Dependency collector interface
add_library(editor_lib INTERFACE)
target_link_libraries(editor_lib INTERFACE SDL2::SDL2 imgui imgui_fd ${OPENMPT_LIBRARIES} spdlog)
target_include_directories(editor_lib INTERFACE ${OPENMPT_INCLUDE_DIRS})
target_compile_options(editor_lib PRIVATE ${OPENMPT_CFLAGS_OTHER})

# Editor
add_executable(editor src/editor/main.cc)
target_link_libraries(editor PRIVATE editor_lib)

# Proofs of concept
add_executable(dummy_sdl_win src/poc/dummy_sdl_win.cc)
target_link_libraries(dummy_sdl_win PRIVATE SDL2::SDL2 SDL2::SDL2main)
