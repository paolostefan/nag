/**
 * @file graph_editor_main.cc
 * @brief Simple entry point demonstrating how to use the GraphEditorUI component
 *
 * This file shows how to instantiate and run a standalone graph editor window
 * using the GraphEditorUI class, which combines UIWindow and GraphEditor.
 */

#include "editor/graph_editor_ui.h"

int main(int, char **) {
  try {
    // Create and run the graph editor UI
    // GraphEditorUI combines:
    // - UIWindow: handles SDL2/OpenGL window and ImGui rendering loop
    // - GraphEditor: provides node graph logic and operations
    // Run the event loop (returns when window closes)
    return GraphEditorUI().run();
  } catch (const std::exception &e) {
    spdlog::critical("Failed to start graph editor: {}", e.what());
    return -1;
  }
}

