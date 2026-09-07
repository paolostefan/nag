# GraphEditorUI -- Node Graph Editor Component

## Overview

`GraphEditorUI` is a complete component class for node graph editing. It
combines:

- **UIWindow**: SDL2/OpenGL window management and ImGui rendering
    loop\
- **GraphEditor**: node graph logic, CRUD operations, serialization

## Features

✅ **Visual editor** --- ImNodes interface to create/connect nodes\
✅ **Undo/Redo** --- full command history\
✅ **Save/Load** --- JSON graph persistence\
✅ **Preview window** --- visualization of visual node outputs\
✅ **Context menu** --- create nodes by category\
✅ **Node properties** --- panel to edit node parameters\
✅ **Keyboard shortcuts** --- Delete, Undo, Redo, Pause

## Usage

### Simple usage

``` cpp
#include "editor/graph_editor_ui.h"

int main() {
    GraphEditorUI editor;
    editor.build_default_graph();
    return editor.run();
}
```

### Integration into a larger UI

``` cpp
class MainEditor : public UIWindow {
private:
    GraphEditorUI graph_editor_;
    
    void render_ui() override {
        // Managed internally
    }
};
```

## Class Hierarchy

    UIWindow (base)
        ↑
        └─ GraphEditorUI (final class)
               ↑
               └─ GraphEditor (logic mixin)

## Main API

### Constructor

``` cpp
GraphEditorUI();  // Creates 1024x768 window titled "Graph Editor"
```

### Public Methods

  Method                    Description
  ------------------------- ----------------------------------
  `run()`                   Starts main loop
  `build_default_graph()`   Creates a sample connected graph

### Protected Methods

  Method                         Description
  ------------------------------ ---------------------------------------
  `spawn_node(type, position)`   Creates a new node at screen position
  `save_graph(path)`             Saves current graph to JSON
  `load_graph(path)`             Loads graph from JSON
  `delete_nodes(ids)`            Deletes nodes by ID
  `delete_links(ids)`            Deletes links by ID

## Performance

- **Rendering**: \~60 FPS typical (ImGui)
- **Graph evaluation**: Single-threaded
- **Memory**: \~1--2MB per medium graph (100+ nodes)

## Limitations

- No multi-threading
- No temporal plotting
- No node copy/paste
- Standard ImNodes zoom/pan only
