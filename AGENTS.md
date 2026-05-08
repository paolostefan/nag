# Nag - SDL2 Demo Engine

## Project Overview

Nag is a visual node graph-based demo engine written in C++20. It allows creating real-time visual effects through a node graph editor, with support for audio playback via OpenMPT and mpg123.

## Tech Stack

- **Language**: C++20
- **Graphics**: SDL2 + OpenGL/GLEW + ImGui (docking branch)
- **Node Graph UI**: ImNodes + ImSequencer (custom)
- **Audio**: mpg123, OpenMPT
- **Serialization**: nlohmann/json
- **Logging**: spdlog
- **Build**: CMake 3.20+ (Linux-only as of 2026)

## Project Structure

```
src/
├── engine/          # Core engine: node graph, rendering, effects
│   ├── nodes/       # Node type definitions (fx, math, generator, etc.)
│   ├── *.cc         # Core engine files
│   └── node_*.cc    # Node registries
├── editor/          # Editor application
│   ├── graph_editor_ui.cc    # Main editor UI combining window + graph
│   ├── graph_editor.cc       # Graph logic (CRUD, serialization)
│   ├── node_properties_panel.cc
│   ├── preview_window.cc
│   ├── scene.cc/h             # Scene data model (folders, graphs, timeline)
│   └── scene_library.cc/h    # Scene file operations
├── fx/              # Effect implementations
├── shaders/         # GLSL shaders
├── include/         # Public headers
└── poc/             # Proofs of concept
```

## Scene Editor Component (WIP)

A unified editor combining node graph editing with timeline-based scene organization.

### Data Structures (src/include/editor/scene.h)
- `GraphReference`: Reference to a saved graph (id, name, path, dirty flag)
- `GraphFolder`: Folder with nested children for organizing graphs
- `TimelineSegment`: Timeline item referencing a graph (frame_start, frame_end, graph_id, color)
- `Scene`: Root container (name, folders, timeline, fps, total_frames)

### File Format
- **Graph files**: `.nag` (JSON via `JsonGraphSerializer`)
- **Scene file**: `.nagscene` (JSON with folder structure + timeline)

### Key Classes
- `Scene`: CRUD for folders, graphs, timeline segments
- `SceneLibrary`: File operations (save/load scenes and graphs)
- `SceneEditorUI`: Combined UIWindow + GraphEditor + ImSequencer::SequenceInterface

### UI Components
- **Graph Library Panel**: Tree view of folders with expand/collapse, context menus
- **Timeline**: ImSequencer integration with TimelineSegment items
- **Node Graph**: ImNodes editor with context menu for node creation

### Building the Scene Editor
```bash
./cmake-build-debug/scene_editor
```

## Key Conventions

### Code Style
- Header files (.h) contain declarations only
- Implementation files (.cc) contain definitions
- Node types are registered via registries (fx_node_registry, etc.)
- Use `std::unique_ptr` for owned resources

### Node Graph Architecture
- `NodeRegistry` handles node type registration and creation
- `NodeGraph` manages nodes (`std::vector<std::unique_ptr<Node>>`) and links (connections)
- `Node` base class with virtual `evaluate()` method
- `NodeType` enum (must append-only before `Count`, never reorder — enforced by static_assert on `kNodeTypeNames`)
- `MultiInputNode` extends Node with dynamic alphabet-labelled inputs (a–z)
- JSON serialization via `JsonGraphSerializer`

### Pins & Streams
- **`Pin`**: has `id`, `direction` (Input/Output), `name`, `data_type` (`const std::type_info*`), and `stream` (`std::shared_ptr<StreamBase>`). Input pins also track `last_seen_version` for change detection.
- **`Stream<T>`** (extends `StreamBase`): templated data carrier holding `T value{}` and a `uint64_t version` counter incremented on every `update()`.
- **Linking**: `NodeGraph::add_link()` shares the output pin's `Stream<T>` `shared_ptr` with the input pin. Both pins then point to the exact same stream instance — enabling fan-out.
- **Unlinking**: `NodeGraph::remove_link()` sets the input pin's `stream` back to `nullptr`.
- **Type safety**: `Pin::try_get_value<T>()` checks `type_info` at runtime before casting. Nodes use `dynamic_cast<Stream<T>*>` on `pin.stream.get()` in `evaluate()`.
- **Data types**: `Stream<float>` (generators, math), `Stream<bool>` (CompareNode), `Stream<Texture*>` (visual pipeline — VisualNode renders to FBO and outputs Texture*).

### Graph Evaluation
- Uses Kahn's algorithm (topological sort) in `NodeGraph::evaluate()`.
- Nodes are processed in dependency order; each node's `evaluate()` reads from input streams and writes to output streams.
- `needs_evaluation()` checks if any input stream's `version` differs from the pin's `last_seen_version` (skips re-processing unchanged data).
- `OutputNode` is the sink: reads `Stream<Texture*>` input, stores the `Texture*` for preview rendering.
- `TimeNode` is the source: outputs a `Stream<float>` driven by external time updates.

### Editor Architecture
- `GraphEditor` = node graph logic (mixin)
- `GraphEditorUI` = `UIWindow` + `GraphEditor` combined
- `UIWindow` handles SDL2/OpenGL window + ImGui render loop
- Command pattern for undo/redo via `CommandHistory`

### ImGui Integration
- Use ImGui's docking branch features
- Context menus via `ImGui::BeginPopup()`
- Property panels as docked windows

## Building

```bash
mkdir cmake-build-debug && cd cmake-build-debug
cmake ..
make -j$(nproc)
./src/editor/editor
```

**System dependencies (APT)**:
- libasound2-dev, libglew-dev, libmpg123-dev, libopenmpt-dev, pkg-config

**CMake-managed dependencies** (downloaded automatically):
- SDL2, ImGui, ImGuiFileDialog, ImPlot, nlohmann/json, spdlog, stb

## Known Issues / Active Development

- There is no real main "launcher" - you must run the graph editor binary directly
- Audio buzzing when slider moved while playing
- Audio track UI is cluttered
- Timeline allows deleting all tracks (should protect effects)
- Keyframe handling is incomplete
- Effect parameter keyframing via UI not implemented

## Commands

- Delete: Remove selected nodes/links
- Ctrl+Z: Undo
- Ctrl+Y: Redo
- Space: Pause/Resume
