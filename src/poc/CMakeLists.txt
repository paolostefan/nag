cmake_minimum_required(VERSION 3.20)

add_executable(dummy_sdl_win dummy_sdl_win.cc)
target_link_libraries(dummy_sdl_win PRIVATE SDL2::SDL2 SDL2::SDL2main)

# Graph Editor (standalone, using GraphEditorUI component)
add_executable(graph_editor graph_editor_main.cc)
target_link_libraries(graph_editor PRIVATE nag_editor_lib)
