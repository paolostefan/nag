cmake_minimum_required(VERSION 3.20)
project(nag LANGUAGES CXX)

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "dependencies")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for clangd and AI assistant tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SDL2)

# Imgui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui)

## ImGui requirement

find_package(Freetype REQUIRED)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

set(IMGUI_ENABLE_FREETYPE ON)

target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL Freetype::Freetype)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/)

add_library(imgui::imgui ALIAS imgui)

# Imgui filedialog
set(IGFD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    imgui_filedialog
    GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui_filedialog)

#Message(STATUS "Imgui file dialog src dir: ${imgui_filedialog_SOURCE_DIR}")

# ImPlot
FetchContent_Declare(
    implot_repo
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(implot_repo)

#Message(STATUS "Implot src dir: ${implot_repo_SOURCE_DIR}")

add_library(implot STATIC
    ${implot_repo_SOURCE_DIR}/implot.h
    ${implot_repo_SOURCE_DIR}/implot_internal.h
    ${implot_repo_SOURCE_DIR}/implot.cpp
    ${implot_repo_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot PUBLIC ${implot_repo_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG develop
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(nlohmann_json)

# OpenMPT
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENMPT REQUIRED libopenmpt)

if (NOT OPENMPT_FOUND)
  message(ERROR "OpenMPT not found")
endif ()

# Spdlog
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v2.x
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(spdlog)

# IconFontCppHeaders
FetchContent_Declare(
    iconfont_cppheaders
    GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
    GIT_TAG main
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(iconfont_cppheaders)
#Message(STATUS "iconfont_cppheaders source dir: ${iconfont_cppheaders_SOURCE_DIR}")

# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(stb)

# ============================================================================
# END DEPENDENCIES
# ============================================================================

# Nag Engine

# Begin shader embedding
# Collect all shaders to embed them as resources in the executable
file(GLOB_RECURSE SHADER_FILES src/shaders/*.vert src/shaders/*.frag src/shaders/*.geom)
set(GENERATED_SHADER_HEADERS "")

foreach (SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    string(REPLACE "." "_" VARIABLE_NAME ${SHADER_NAME})
    set(OUTPUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/shaders/${VARIABLE_NAME}.h")

    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${SHADER_FILE}
            -DOUTPUT_FILE=${OUTPUT_HEADER}
            -DVARIABLE_NAME=k${VARIABLE_NAME}
            -P ${CMAKE_SOURCE_DIR}/cmake/embed_shader.cmake
        DEPENDS ${SHADER_FILE}
        COMMENT "Embedding shader: ${SHADER_NAME}"
    )

    list(APPEND GENERATED_SHADER_HEADERS ${OUTPUT_HEADER})
endforeach()
# End shader embedding

file(GLOB ENGINE_SRCS src/engine/*.cc src/engine/nodes/*.h src/fx/*.cc)

add_library(nag_engine_lib
    ${ENGINE_SRCS}
    src/engine/fx_node_registry.cc
    src/engine/generator_nodes_registry.cc
    src/engine/math_nodes_registry.cc
    src/engine/node_graph.cc
    src/engine/node_registry.cc
    src/engine/render_target.cc
    src/engine/serialization/json_graph_serializer.cc
    src/engine/shader_quad_helper.cc
    src/engine/temporal_nodes_registry.cc
    src/engine/visual_nodes_registry.cc
    ${GENERATED_SHADER_HEADERS}
    src/engine/property_widget.cc
)

target_include_directories(nag_engine_lib PUBLIC
    src/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${iconfont_cppheaders_SOURCE_DIR}/
    ${imgui_filedialog_SOURCE_DIR}/
    ${OPENMPT_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
)

target_link_libraries(nag_engine_lib PUBLIC
    OpenGL::GL
    GLEW::GLEW
    SDL2::SDL2
    imgui
    mpg123
    spdlog
    ${OPENMPT_LIBRARIES}
    nlohmann_json::nlohmann_json
)

target_compile_options(nag_engine_lib PRIVATE
    ${OPENMPT_CFLAGS_OTHER}
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
#    -Wold-style-cast
    -Woverloaded-virtual
)

# Editor library
add_library(nag_editor_lib
    ${imgui_filedialog_SOURCE_DIR}/ImGuiFileDialog.cpp
    src/_3rd_party/ImGuizmo/ImSequencer.cpp
    src/_3rd_party/imnodes/imnodes.cpp
    src/editor/command_history.cc
    src/editor/graph_commands.cc
    src/editor/graph_editor_ui.cc
    src/editor/node_properties_panel.cc
    src/editor/preview_window.cc
    src/editor/scene.cc
    src/editor/scene_library.cc
    src/editor/toast_manager.cc
    src/editor/ui_window.cc
    src/editor/graph_editor.cc
    src/engine/stb_impl.cc
)

target_include_directories(nag_editor_lib PUBLIC
    src/include
    src/_3rd_party/ImGuizmo
    src/_3rd_party/imnodes
)

target_link_libraries(nag_editor_lib PUBLIC
    nag_engine_lib
    imgui::imgui
    implot
)

# Scene Editor
add_executable(scene_editor
    src/editor/scene_editor_main.cc
    src/editor/scene_editor_ui.cc
)
target_link_libraries(scene_editor PRIVATE nag_editor_lib)

# Proofs of concept
add_subdirectory(src/poc)

# Enable testing
add_subdirectory(tests)