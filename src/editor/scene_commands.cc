#include "editor/scene_commands.h"

#include "spdlog/spdlog.h"

#include "editor/scene.h"

namespace {
  struct FolderLocation {
    std::vector<std::unique_ptr<GraphFolder> > *parent{nullptr};
    size_t index{0};
    std::string parent_folder_id; // Empty = root
  };

  bool locate_folder(std::vector<std::unique_ptr<GraphFolder> > &folders,
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
      if (locate_folder(folders[i]->children, id, folders[i]->id, out)) {
        return true;
      }
    }
    return false;
  }

  bool locate_graph(std::vector<std::unique_ptr<GraphFolder> > &folders,
                    const std::string &graph_id,
                    FolderLocation &out) {
    for (size_t i = 0; i < folders.size(); ++i) {
      for (size_t g = 0; g < folders[i]->graphs.size(); ++g) {
        if (folders[i]->graphs[g].id == graph_id) {
          out.parent = &folders;
          out.index = g;
          out.parent_folder_id = folders[i]->id; // The folder containing the graph
          return true;
        }
      }
      if (locate_graph(folders[i]->children, graph_id, out)) {
        return true;
      }
    }
    return false;
  }
} // namespace

// ============================================================================
// AddFolderCommand
// ============================================================================

AddFolderCommand::AddFolderCommand(std::string parent_folder_id, std::string folder_name)
  : parent_folder_id_(std::move(parent_folder_id)), folder_name_(std::move(folder_name)) {
}

bool AddFolderCommand::execute(Scene &scene) {
  if (created_folder_id_.empty()) {
    created_folder_id_ = Scene::generate_id();
  }
  auto folder = std::make_unique<GraphFolder>(created_folder_id_, folder_name_);
  if (parent_folder_id_.empty()) {
    scene.root_folders.push_back(std::move(folder));
  } else {
    GraphFolder *parent = scene.find_folder(parent_folder_id_);
    if (!parent) {
      spdlog::error("AddFolderCommand: parent folder not found: {}", parent_folder_id_);
      return false;
    }
    parent->children.push_back(std::move(folder));
  }
  scene.dirty = true;
  return true;
}

bool AddFolderCommand::undo(Scene &scene) {
  return scene.remove_folder(created_folder_id_);
}

std::string AddFolderCommand::description() const {
  return "Add folder \"" + folder_name_ + "\"";
}

// ============================================================================
// RemoveFolderCommand
// ============================================================================

RemoveFolderCommand::RemoveFolderCommand(const std::string &folder_id)
  : folder_id_(folder_id) {
}

bool RemoveFolderCommand::execute(Scene &scene) {
  if (!captured_) {
    FolderLocation loc;
    if (!locate_folder(scene.root_folders, folder_id_, "", loc)) {
      spdlog::error("RemoveFolderCommand: folder not found: {}", folder_id_);
      return false;
    }
    parent_folder_id_ = loc.parent_folder_id;
    index_ = loc.index;
    folder_json_ = *loc.parent->at(loc.index);
    captured_ = true;
  }

  scene.remove_folder(folder_id_);
  return true;
}

bool RemoveFolderCommand::undo(Scene &scene) {
  if (folder_json_.is_null()) {
    return false;
  }

  auto folder = std::make_unique<GraphFolder>();
  from_json(folder_json_, *folder);

  GraphFolder *parent = parent_folder_id_.empty() ? nullptr : scene.find_folder(parent_folder_id_);
  auto &siblings = parent ? parent->children : scene.root_folders;
  const auto pos = siblings.begin() + static_cast<long>(std::min(index_, siblings.size()));
  siblings.insert(pos, std::move(folder));
  scene.dirty = true;
  return true;
}

std::string RemoveFolderCommand::description() const {
  return "Remove folder \"" + folder_id_ + "\"";
}

// ============================================================================
// RenameFolderCommand
// ============================================================================

RenameFolderCommand::RenameFolderCommand(std::string folder_id, std::string new_name)
  : folder_id_(std::move(folder_id)), new_name_(std::move(new_name)) {
}

bool RenameFolderCommand::execute(Scene &scene) {
  if (old_name_.empty()) {
    const GraphFolder *folder = scene.find_folder(folder_id_);
    if (!folder) {
      return false;
    }
    old_name_ = folder->name;
  }
  return scene.rename_folder(folder_id_, new_name_);
}

bool RenameFolderCommand::undo(Scene &scene) {
  return scene.rename_folder(folder_id_, old_name_);
}

std::string RenameFolderCommand::description() const {
  return "Rename folder to \"" + new_name_ + "\"";
}

// ============================================================================
// AddGraphCommand
// ============================================================================

AddGraphCommand::AddGraphCommand(std::string folder_id, std::string graph_name)
  : folder_id_(std::move(folder_id)), graph_name_(std::move(graph_name)), graph_id_(Scene::generate_id()) {
}

bool AddGraphCommand::execute(Scene &scene) {
  GraphFolder *folder = scene.find_folder(folder_id_);
  if (!folder) {
    spdlog::error("AddGraphCommand: folder not found: {}", folder_id_);
    return false;
  }
  if (scene.find_graph(graph_id_)) {
    return true; // Already present (redo)
  }
  folder->graphs.emplace_back(graph_id_, graph_name_);
  scene.graph_data[graph_id_] = nullptr;
  scene.dirty = true;
  return true;
}

bool AddGraphCommand::undo(Scene &scene) {
  return scene.remove_graph(graph_id_);
}

std::string AddGraphCommand::description() const {
  return "Add graph \"" + graph_name_ + "\"";
}

// ============================================================================
// RemoveGraphCommand
// ============================================================================

RemoveGraphCommand::RemoveGraphCommand(const std::string &graph_id)
  : graph_id_(graph_id) {
}

bool RemoveGraphCommand::execute(Scene &scene) {
  if (!captured_) {
    FolderLocation loc;
    if (!locate_graph(scene.root_folders, graph_id_, loc)) {
      spdlog::error("RemoveGraphCommand: graph not found: {}", graph_id_);
      return false;
    }
    parent_folder_id_ = loc.parent_folder_id;
    index_ = loc.index;
    graph_ref_json_ = loc.parent->at(loc.index)->graphs[loc.index];
    if (const auto it = scene.graph_data.find(graph_id_); it != scene.graph_data.end()) {
      graph_data_ = it->second;
      has_graph_data_ = true;
    }
    captured_ = true;
  }

  scene.remove_graph(graph_id_);
  return true;
}

bool RemoveGraphCommand::undo(Scene &scene) {
  if (graph_ref_json_.is_null()) {
    return false;
  }

  GraphReference ref;
  from_json(graph_ref_json_, ref);

  GraphFolder *parent = scene.find_folder(parent_folder_id_);
  if (!parent) {
    spdlog::error("RemoveGraphCommand::undo: parent folder not found: {}", parent_folder_id_);
    return false;
  }
  std::vector<GraphReference> &graphs = parent->graphs;
  const auto pos = graphs.begin() + static_cast<long>(std::min(index_, graphs.size()));
  graphs.insert(pos, ref);
  if (has_graph_data_) {
    scene.graph_data[graph_id_] = graph_data_;
  }
  scene.dirty = true;
  return true;
}

std::string RemoveGraphCommand::description() const {
  return "Remove graph \"" + graph_id_ + "\"";
}

// ============================================================================
// RenameGraphCommand
// ============================================================================

RenameGraphCommand::RenameGraphCommand(std::string graph_id, std::string new_name)
  : graph_id_(std::move(graph_id)), new_name_(std::move(new_name)) {
}

bool RenameGraphCommand::execute(Scene &scene) {
  if (old_name_.empty()) {
    const GraphReference *graph = scene.find_graph(graph_id_);
    if (!graph) {
      return false;
    }
    old_name_ = graph->name;
  }
  return scene.rename_graph(graph_id_, new_name_);
}

bool RenameGraphCommand::undo(Scene &scene) {
  return scene.rename_graph(graph_id_, old_name_);
}

std::string RenameGraphCommand::description() const {
  return "Rename graph to \"" + new_name_ + "\"";
}

// ============================================================================
// AddTimelineSegmentCommand
// ============================================================================

AddTimelineSegmentCommand::AddTimelineSegmentCommand(TimelineSegment segment)
  : segment_(std::move(segment)) {
}

bool AddTimelineSegmentCommand::execute(Scene &scene) {
  index_ = scene.timeline.size();
  scene.add_timeline_segment(segment_);
  return true;
}

bool AddTimelineSegmentCommand::undo(Scene &scene) {
  if (index_ >= scene.timeline.size()) {
    return false;
  }
  scene.remove_timeline_segment(index_);
  return true;
}

std::string AddTimelineSegmentCommand::description() const {
  return "Add timeline segment";
}

// ============================================================================
// RemoveTimelineSegmentCommand
// ============================================================================

RemoveTimelineSegmentCommand::RemoveTimelineSegmentCommand(const size_t index)
  : index_(index) {
}

bool RemoveTimelineSegmentCommand::execute(Scene &scene) {
  if (index_ >= scene.timeline.size()) {
    spdlog::error("RemoveTimelineSegmentCommand: invalid index {}", index_);
    return false;
  }
  segment_ = scene.timeline[index_];
  scene.remove_timeline_segment(index_);
  return true;
}

bool RemoveTimelineSegmentCommand::undo(Scene &scene) {
  const auto pos = scene.timeline.begin() + static_cast<long>(std::min(index_, scene.timeline.size()));
  scene.timeline.insert(pos, segment_);
  scene.dirty = true;
  return true;
}

std::string RemoveTimelineSegmentCommand::description() const {
  return "Remove timeline segment";
}

// ============================================================================
// ImportAudioCommand
// ============================================================================

ImportAudioCommand::ImportAudioCommand(AudioTrack track)
  : track_(std::move(track)),
    path_(track_.path.string()),
    start_seconds_(track_.start_seconds),
    title_(track_.title) {
}

bool ImportAudioCommand::execute(Scene &scene) {
  // A player is only missing when the track was moved out by the first execute
  // (undo); rebuild it so redo can re-import the file.
  if (!track_.player && !path_.empty()) {
    try {
      track_ = AudioTrack(path_, start_seconds_);
    } catch (const std::exception &e) {
      spdlog::error("ImportAudioCommand: failed to reload track '{}': {}", path_, e.what());
      return false;
    }
  }
  if (!track_.player) {
    return false;
  }

  track_index_ = scene.add_audio_track(std::move(track_));
  segment_index_ = scene.timeline.size();
  const TimelineSegment segment(0, 100, static_cast<int>(track_index_));
  scene.add_timeline_segment(segment);
  return true;
}

bool ImportAudioCommand::undo(Scene &scene) {
  if (segment_index_ < scene.timeline.size()) {
    scene.remove_timeline_segment(segment_index_);
  }
  scene.remove_audio_track(track_index_);
  return true;
}

std::string ImportAudioCommand::description() const {
  return "Import audio track \"" + title_ + "\"";
}