#ifndef NAG_EDITOR_NODE_PROPERTIES_RENDERERS_H
#define NAG_EDITOR_NODE_PROPERTIES_RENDERERS_H

struct Node;
class NodeGraph;
class CommandHistory;

void draw_node_properties(Node &node, NodeGraph &graph, CommandHistory &history);

// Editor adapter: TextureLoaderNode's file loading is engine-side (path
// string); the ImGui file dialog lives here so the engine stays GUI-free.
struct TextureLoaderNode;

void open_file_dialog(TextureLoaderNode &node);

bool display_file_dialog(TextureLoaderNode &node);

#endif //NAG_EDITOR_NODE_PROPERTIES_RENDERERS_H
