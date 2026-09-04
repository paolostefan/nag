#include "editor/graph_editor.h"

#include "editor/graph_commands.h"
#include "engine/nodes/visual_node.h"

GraphEditor::GraphEditor() {
  register_all_builtin_nodes();

  GraphEditor::build_default_graph();
}

void GraphEditor::reset_graph() {
  // invalidate pointers to clear references
  time_node = nullptr;
  output_node = nullptr;

  graph.clear();

  node_pos_refresh.store(true, std::memory_order_release);
}

void GraphEditor::build_default_graph() {
  // No default graph for now, just start with time and output nodes

  // Time node
  auto time_node_unique = TimeNode::create();
  time_node_unique->gui_x = 30;
  time_node_unique->gui_y = 100;
  time_node = dynamic_cast<TimeNode *>(graph.add_node(std::move(time_node_unique)));

  // Visual: OutputNode (sink) — receives the composited texture
  auto output_node_unique = OutputNode::create();
  output_node_unique->gui_x = 300;
  output_node_unique->gui_y = 100;
  output_node = dynamic_cast<OutputNode *>(graph.add_node(std::move(output_node_unique)));
}

OperationResult GraphEditor::load_graph(const std::string &path) {
  // load a temp graph
  NodeGraph temp_graph;

  if (const auto result = serializer.load(temp_graph, path); !result) {
    spdlog::error("Graph load failed: {}", result.error_message);
    return result;
  }

  // Commit: replace current graph
  reset_graph();
  graph = std::move(temp_graph);

  // search time & output nodes in the existing graph
  for (const auto &node: graph.nodes) {
    if (auto *t = dynamic_cast<TimeNode *>(node.get())) {
      time_node = t;
    }
    if (auto *o = dynamic_cast<OutputNode *>(node.get())) {
      output_node = o;
    }
  }

  return OperationResult::ok();
}

Node *GraphEditor::spawn_node(const NodeType type, const float gui_x, const float gui_y) {
  auto node = NodeRegistry::instance().create_node(type);
  if (!node) {
    spdlog::error("Failed to create node of type {}", static_cast<int>(type));
    return nullptr;
  }

  // Init visual nodes with default size
  if (auto *visual = dynamic_cast<VisualNode *>(node.get())) {
    // TODO: remove hardcoded default size
    if (!visual->initialize(400, 300)) {
      spdlog::warn("Visual node '{}' failed to initialize render target",
                   node->name);
    }
  }

  node->gui_x = gui_x;
  node->gui_y = gui_y;

  auto add_command = std::make_unique<AddNodeCommand>(std::move(node));
  if (history_enabled.load(std::memory_order_acquire)) {
    command_history.execute(graph, std::move(add_command));
  } else {
    add_command->execute(graph);
  }

  // ReSharper disable once CppDFALocalValueEscapesFunction
  return node.get();
}

void GraphEditor::delete_nodes(const std::unordered_set<int> &node_ids_to_delete) {
  if (node_ids_to_delete.empty()) return;

  // ---- Step 1: Invalidate time/output pointers if deleted -------------
  for (const auto &node: graph.nodes) {
    if (!node_ids_to_delete.contains(node->id)) continue;

    // Special case: TimeNode
    if (node.get() == time_node) {
      time_node = nullptr;
    }

    // Special case: OutputNode
    if (node.get() == output_node) {
      output_node = nullptr;
    }
  }

  // ---- Step 2: Remove the nodes themselves ------------------------------

  auto del_command = std::make_unique<DeleteNodesCommand>(node_ids_to_delete);
  command_history.execute(graph, std::move(del_command));

  // ---- Step3: Cleanup orphaned input streams ----------------------------
  // After node deletion, some input pins might still reference deleted
  // output streams. Walk all remaining nodes and null out dead streams.
  for (const auto &node: graph.nodes) {
    for (auto &input_pin: node->inputs) {
      if (!input_pin.stream) continue;

      // Check if this stream still exists in any output pin
      bool stream_exists = false;
      for (const auto &other_node: graph.nodes) {
        for (const auto &output_pin: other_node->outputs) {
          if (output_pin.stream == input_pin.stream) {
            stream_exists = true;
            break;
          }
        }
        if (stream_exists) break;
      }

      if (!stream_exists) {
        input_pin.stream = nullptr;
      }
    }
  }

  spdlog::info("Deleted {} node(s)", node_ids_to_delete.size());
}

void GraphEditor::delete_links(const std::unordered_set<int> &link_ids_to_delete) {
  auto del_command = std::make_unique<DeleteLinksCommand>(link_ids_to_delete);
  command_history.execute(graph, std::move(del_command));

  spdlog::info("Deleted {} link(s)", link_ids_to_delete.size());
}
