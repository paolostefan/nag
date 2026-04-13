#include "editor/scene.h"

#include <functional>
#include <random>
#include <sstream>

std::string Scene::generate_id() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  std::stringstream ss;
  ss << std::hex;
  for (int i = 0; i < 16; ++i) {
    ss << dis(gen);
  }
  return ss.str();
}

GraphFolder *Scene::find_folder(const std::string &folder_id) {
  return const_cast<GraphFolder *>(static_cast<const Scene *>(this)->find_folder(folder_id));
}

const GraphFolder *Scene::find_folder(const std::string &folder_id) const {
  std::function<const GraphFolder *(const std::vector<std::unique_ptr<GraphFolder>> &)> search =
      [&](const std::vector<std::unique_ptr<GraphFolder>> &folders) -> const GraphFolder * {
    for (const auto &folder : folders) {
      if (folder->id == folder_id) {
        return folder.get();
      }
      if (auto found = search(folder->children)) {
        return found;
      }
    }
    return nullptr;
  };
  return search(root_folders);
}

GraphReference *Scene::find_graph(const std::string &graph_id) {
  return const_cast<GraphReference *>(static_cast<const Scene *>(this)->find_graph(graph_id));
}

const GraphReference *Scene::find_graph(const std::string &graph_id) const {
  std::function<const GraphReference *(const std::vector<std::unique_ptr<GraphFolder>> &)> search =
      [&](const std::vector<std::unique_ptr<GraphFolder>> &folders) -> const GraphReference * {
    for (const auto &folder : folders) {
      for (const auto &graph : folder->graphs) {
        if (graph.id == graph_id) {
          return &graph;
        }
      }
      if (auto found = search(folder->children)) {
        return found;
      }
    }
    return nullptr;
  };
  return search(root_folders);
}

GraphFolder *Scene::add_folder(const std::optional<std::string> &parent_id, const std::string &name) {
  auto folder = std::make_unique<GraphFolder>(generate_id(), name);

  if (!parent_id.has_value()) {
    root_folders.push_back(std::move(folder));
    return root_folders.back().get();
  }

  GraphFolder *parent = find_folder(parent_id.value());
  if (!parent) {
    spdlog::error("Parent folder not found: {}", parent_id.value());
    return nullptr;
  }

  parent->children.push_back(std::move(folder));
  pristine = false;
  return parent->children.back().get();
}

bool Scene::remove_folder(const std::string &folder_id) {
  std::function<bool(std::vector<std::unique_ptr<GraphFolder>> &, const std::string &)> remove_from_vector;
  remove_from_vector = [&remove_from_vector](std::vector<std::unique_ptr<GraphFolder>> &folders,
                                             const std::string &id) -> bool {
    for (auto it = folders.begin(); it != folders.end(); ++it) {
      if ((*it)->id == id) {
        folders.erase(it);
        return true;
      }
      if (remove_from_vector((*it)->children, id)) {
        return true;
      }
    }
    return false;
  };

  if (remove_from_vector(root_folders, folder_id)) {
    pristine = false;
    return true;
  }
  return false;
}

GraphReference *Scene::add_graph(const std::string &folder_id,
                                const std::string &graph_name,
                                const NodeGraph & /* graph */) {
  GraphFolder *folder = find_folder(folder_id);
  if (!folder) {
    spdlog::error("Folder not found: {}", folder_id);
    return nullptr;
  }

  std::string id = generate_id();
  std::string filename = id + ".nag";
  std::filesystem::path nag_path = std::filesystem::path(folder_id) / filename;

  folder->graphs.emplace_back(id, graph_name, nag_path);
  pristine = false;
  return &folder->graphs.back();
}

bool Scene::remove_graph(const std::string &graph_id) {
  std::function<bool(std::vector<std::unique_ptr<GraphFolder>> &, const std::string &)> remove_from_vector;
  remove_from_vector = [&remove_from_vector](const std::vector<std::unique_ptr<GraphFolder>> &folders,
                                              const std::string &id) -> bool {
    for (auto &folder : folders) {
      for (auto it = folder->graphs.begin(); it != folder->graphs.end(); ++it) {
        if (it->id == id) {
          folder->graphs.erase(it);
          return true;
        }
      }
      if (remove_from_vector(folder->children, id)) {
        return true;
      }
    }
    return false;
  };

  if (remove_from_vector(root_folders, graph_id)) {
    pristine = false;
    return true;
  }
  return false;
}

bool Scene::move_graph(const std::string &graph_id, const std::string &target_folder_id) {
  GraphReference *graph_ref = find_graph(graph_id);
  if (!graph_ref) {
    spdlog::error("Graph not found: {}", graph_id);
    return false;
  }

  GraphFolder *target_folder = find_folder(target_folder_id);
  if (!target_folder) {
    spdlog::error("Target folder not found: {}", target_folder_id);
    return false;
  }

  std::function<bool(std::vector<std::unique_ptr<GraphFolder>> &, const std::string &, GraphFolder *, GraphReference &&)>
      remove_and_insert;
  remove_and_insert = [&remove_and_insert](std::vector<std::unique_ptr<GraphFolder>> &folders,
                                            const std::string &id, GraphFolder *target,
                                            GraphReference &&ref) -> bool {
    for (auto &folder : folders) {
      for (auto it = folder->graphs.begin(); it != folder->graphs.end(); ++it) {
        if (it->id == id) {
          target->graphs.push_back(std::move(*it));
          folder->graphs.erase(it);
          return true;
        }
      }
      if (remove_and_insert(folder->children, id, target, std::move(ref))) {
        return true;
      }
    }
    return false;
  };

  GraphReference copy = *graph_ref;
  if (remove_and_insert(root_folders, graph_id, target_folder, std::move(copy))) {
    pristine = false;
    return true;
  }
  return false;
}

bool Scene::rename_graph(const std::string &graph_id, const std::string &new_name) {
  GraphReference *graph = find_graph(graph_id);
  if (!graph) {
    spdlog::error("Graph not found: {}", graph_id);
    return false;
  }
  graph->name = new_name;
  pristine = false;
  return true;
}

void Scene::add_timeline_segment(const TimelineSegment &segment) {
  timeline.push_back(segment);
  pristine = false;
}

void Scene::remove_timeline_segment(size_t index) {
  if (index < timeline.size()) {
    timeline.erase(timeline.begin() + static_cast<long>(index));
    pristine = false;
  }
}

void Scene::update_timeline_segment(size_t index, const TimelineSegment &segment) {
  if (index < timeline.size()) {
    timeline[index] = segment;
    pristine = false;
  }
}

int Scene::get_frame_at(int frame) const {
  for (size_t i = 0; i < timeline.size(); ++i) {
    const auto &segment = timeline[i];
    if (frame >= segment.frame_start && frame < segment.frame_end) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

const GraphReference *Scene::get_graph_at_frame(int frame) const {
  int segment_index = get_frame_at(frame);
  if (segment_index < 0) {
    return nullptr;
  }
  return find_graph(timeline[static_cast<size_t>(segment_index)].graph_id);
}
