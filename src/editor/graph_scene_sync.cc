#include "editor/graph_scene_sync.h"

#include <functional>

#include "spdlog/spdlog.h"

GraphReference *GraphSceneSync::find_first_graph() const {
  std::function<GraphReference *(const std::vector<std::unique_ptr<GraphFolder> > &)> find_first;
  find_first = [&find_first](const std::vector<std::unique_ptr<GraphFolder> > &folders) -> GraphReference * {
    for (const auto &folder: folders) {
      if (!folder->graphs.empty()) {
        return &folder->graphs.front();
      }
      if (auto *found = find_first(folder->children)) {
        return found;
      }
    }
    return nullptr;
  };
  return find_first(scene_.root_folders);
}

void GraphSceneSync::sync_graph(GraphReference &ref, const NodeGraph &graph) {
  scene_.graph_data[ref.id] = JsonGraphSerializer::serialize_graph(graph);
  ref.dirty = false;
}

std::optional<NodeGraph> GraphSceneSync::load_graph(const GraphReference &ref) const {
  const auto it = scene_.graph_data.find(ref.id);
  if (it == scene_.graph_data.end() || it->second.is_null()) {
    return std::nullopt;
  }

  NodeGraph loaded_graph;
  const auto result = JsonGraphSerializer::deserialize_graph(loaded_graph, it->second);
  if (!result) {
    spdlog::error("Failed to deserialize graph '{}': {}",
                  ref.name, result.error_message);
    return std::nullopt;
  }
  return loaded_graph;
}

GraphReference *GraphSceneSync::add_graph(GraphFolder &folder, const std::string_view graph_name) {
  auto *ref = scene_.add_graph(folder, std::string(graph_name));
  if (!ref) return nullptr;
  scene_.graph_data[ref->id] = nullptr;
  return ref;
}

void GraphSceneSync::remove_graph_data(const std::string &graph_id) {
  scene_.graph_data.erase(graph_id);
}
