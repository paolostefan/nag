#include "editor/scene.h"

#include <random>
#include <sstream>

namespace {
  bool locate_folder_in(std::vector<std::unique_ptr<GraphFolder> > &folders,
                        const std::string &id,
                        const std::string &parent_folder_id,
                        FolderLocation &out) {
    for (size_t i = 0; i < folders.size(); ++i) {
      if (folders[i]->id == id) {
        out.parent = &folders;
        out.index = i;
        out.parent_folder_id = parent_folder_id;
        return true;
      }
      if (locate_folder_in(folders[i]->children, id, folders[i]->id, out)) {
        return true;
      }
    }
    return false;
  }

  bool locate_graph_in(std::vector<std::unique_ptr<GraphFolder> > &folders,
                       const std::string &graph_id,
                       GraphLocation &out) {
    for (auto &folder: folders) {
      for (size_t g = 0; g < folder->graphs.size(); ++g) {
        if (folder->graphs[g].id == graph_id) {
          out.parent = &folder->graphs;
          out.index = g;
          out.parent_folder_id = folder->id;
          return true;
        }
      }
      if (locate_graph_in(folder->children, graph_id, out)) {
        return true;
      }
    }
    return false;
  }
} // namespace

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
  const GraphFolder *result = nullptr;
  visit_folders([&](const GraphFolder &folder) {
    if (folder.id == folder_id) {
      result = &folder;
      return true;
    }
    return false;
  });
  return result;
}

GraphReference *Scene::find_graph(const std::string &graph_id) {
  return const_cast<GraphReference *>(static_cast<const Scene *>(this)->find_graph(graph_id));
}

const GraphReference *Scene::find_graph(const std::string &graph_id) const {
  const GraphReference *result = nullptr;
  visit_graphs([&](const GraphReference &graph) {
    if (graph.id == graph_id) {
      result = &graph;
      return true;
    }
    return false;
  });
  return result;
}

GraphReference *Scene::find_first_graph() {
  return const_cast<GraphReference *>(static_cast<const Scene *>(this)->find_first_graph());
}

const GraphReference *Scene::find_first_graph() const {
  const GraphReference *result = nullptr;
  visit_graphs([&](const GraphReference &graph) {
    result = &graph;
    return true;
  });
  return result;
}

bool Scene::locate_folder(const std::string &folder_id, FolderLocation &out) {
  return locate_folder_in(root_folders, folder_id, "", out);
}

bool Scene::locate_graph(const std::string &graph_id, GraphLocation &out) {
  return locate_graph_in(root_folders, graph_id, out);
}

GraphFolder *Scene::add_folder(const char *const parent_folder_id, const std::string &folder_name) {
  auto folder = std::make_unique<GraphFolder>(generate_id(), folder_name);

  if (!parent_folder_id) {
    root_folders.push_back(std::move(folder));
    return root_folders.back().get();
  }

  GraphFolder *parent = find_folder(parent_folder_id);
  if (!parent) {
    spdlog::error("Parent folder not found: {}", parent_folder_id);
    return nullptr;
  }

  parent->children.push_back(std::move(folder));
  dirty = true;
  return parent->children.back().get();
}

bool Scene::rename_folder(const std::string &folder_id, const std::string &new_name) {
  GraphFolder *const folder = find_folder(folder_id);
  if (!folder) {
    spdlog::error("Folder not found: {}", folder_id);
    return false;
  }

  folder->name = new_name;
  dirty = true;
  return true;
}

bool Scene::remove_folder(const std::string &folder_id) {
  FolderLocation loc;
  if (!locate_folder(folder_id, loc) || !loc.parent) {
    return false;
  }
  loc.parent->erase(loc.parent->begin() + static_cast<long>(loc.index));
  dirty = true;
  return true;
}

GraphReference *Scene::add_graph(GraphFolder &folder,
                                 const std::string &graph_name) {
  const std::string graph_id = generate_id();

  folder.graphs.emplace_back(graph_id, graph_name);
  dirty = true;
  return &folder.graphs.back();
}

bool Scene::remove_graph(const std::string &graph_id) {
  GraphLocation loc;
  if (!locate_graph(graph_id, loc) || !loc.parent) {
    return false;
  }
  loc.parent->erase(loc.parent->begin() + static_cast<long>(loc.index));
  graph_data.erase(graph_id);
  dirty = true;
  return true;
}

bool Scene::move_graph(const std::string &graph_id, const std::string &target_folder_id) {
  GraphFolder *target_folder = find_folder(target_folder_id);
  if (!target_folder) {
    spdlog::error("Target folder not found: {}", target_folder_id);
    return false;
  }

  GraphLocation loc;
  if (!locate_graph(graph_id, loc) || !loc.parent) {
    spdlog::error("Graph not found: {}", graph_id);
    return false;
  }
  target_folder->graphs.push_back(std::move(loc.parent->at(loc.index)));
  loc.parent->erase(loc.parent->begin() + static_cast<long>(loc.index));
  dirty = true;
  return true;
}

bool Scene::rename_graph(const std::string &graph_id, const std::string &new_name) {
  GraphReference *graph = find_graph(graph_id);
  if (!graph) {
    spdlog::error("Graph not found: {}", graph_id);
    return false;
  }
  graph->name = new_name;
  dirty = true;
  return true;
}

void Scene::add_timeline_segment(const TimelineSegment &segment) {
  timeline.push_back(segment);
  dirty = true;
}

void Scene::remove_timeline_segment(const size_t index) {
  if (index < timeline.size()) {
    timeline.erase(timeline.begin() + static_cast<long>(index));
    dirty = true;
  }
}

void Scene::update_timeline_segment(const size_t index, const TimelineSegment &segment) {
  if (index < timeline.size()) {
    timeline[index] = segment;
    dirty = true;
  }
}

int Scene::get_frame_at(const int frame) const {
  for (size_t i = 0; i < timeline.size(); ++i) {
    if (const auto &segment = timeline[i];
      frame >= segment.frame_start && frame < segment.frame_end) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

const GraphReference *Scene::get_graph_at_frame(const int frame) const {
  const int segment_index = get_frame_at(frame);
  if (segment_index < 0) {
    return nullptr;
  }
  return find_graph(timeline[static_cast<size_t>(segment_index)].graph_id);
}

size_t Scene::add_audio_track(AudioTrack &&track) {
  audio_tracks.push_back(std::move(track));
  dirty = true;
  return audio_tracks.size() - 1;
}

bool Scene::remove_audio_track(const size_t index) {
  if (index >= audio_tracks.size()) return false;
  audio_tracks.erase(audio_tracks.begin() + static_cast<long>(index));
  for (auto &seg: timeline) {
    if (seg.type == SegmentType::AUDIO && seg.audio_track_index > static_cast<int>(index)) {
      seg.audio_track_index--;
    }
  }
  dirty = true;
  return true;
}

AudioTrack *Scene::get_audio_track(const size_t index) {
  return const_cast<AudioTrack *>(static_cast<const Scene *>(this)->get_audio_track(index));
}

const AudioTrack *Scene::get_audio_track(const size_t index) const {
  if (index >= audio_tracks.size()) return nullptr;
  return &audio_tracks[index];
}
