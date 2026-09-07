#ifndef NAG_EDITOR_SCENE_COMMANDS_H
#define NAG_EDITOR_SCENE_COMMANDS_H

#include <string>

#include "nlohmann/json.hpp"

#include "editor/command.h"
#include "editor/scene.h"
#include "engine/audio_track.h"

/**
 * @brief Scene-targeted undoable commands.
 *
 * Each command overrides @c ICommand::targets_scene() to report true and
 * operates on @c Scene via @c execute/undo(Scene&). The NodeGraph overloads are
 * satisfied for interface completeness and always return false — the
 * @c CommandHistory routes scene commands to the bound scene, so they are
 * never invoked with a graph.
 */
class SceneCommand : public ICommand {
public:
  [[nodiscard]] bool targets_scene() const override {
    return true;
  }

  bool execute(NodeGraph &) override {
    return false;
  }

  bool undo(NodeGraph &) override {
    return false;
  }
};

// ============================================================================
// AddFolderCommand
// ============================================================================

/**
 * @brief Create a folder under a parent (or the root when parent id is empty).
 */
class AddFolderCommand : public SceneCommand {
public:
  AddFolderCommand(std::string parent_folder_id, std::string folder_name);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  std::string parent_folder_id_;
  std::string folder_name_;
  std::string created_folder_id_; // Assigned on first execute, reused for redo
};

// ============================================================================
// RemoveFolderCommand
// ============================================================================

/**
 * @brief Remove a folder and its entire subtree from the library.
 *
 * Captures the serialized subtree on first execute so undo can restore it
 * exactly, including nested folders, graphs, and graph data.
 */
class RemoveFolderCommand : public SceneCommand {
public:
  explicit RemoveFolderCommand(const std::string &folder_id);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  std::string folder_id_;
  std::string parent_folder_id_; // Empty = root
  size_t index_{0};
  nlohmann::json folder_json_;
  bool captured_{false};
};

// ============================================================================
// RenameFolderCommand
// ============================================================================

class RenameFolderCommand : public SceneCommand {
public:
  RenameFolderCommand(std::string folder_id, std::string new_name);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  std::string folder_id_;
  std::string new_name_;
  std::string old_name_;
};

// ============================================================================
// AddGraphCommand
// ============================================================================

/**
 * @brief Create a graph reference (with empty data) inside a folder.
 *
 * The graph id is predetermined at construction so undo/redo stay consistent.
 */
class AddGraphCommand : public SceneCommand {
public:
  AddGraphCommand(std::string folder_id, std::string graph_name);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

  /// @brief The id assigned to the new graph (valid before execution too).
  [[nodiscard]] const std::string &graph_id() const {
    return graph_id_;
  }

private:
  std::string folder_id_;
  std::string graph_name_;
  std::string graph_id_;
};

// ============================================================================
// RemoveGraphCommand
// ============================================================================

/**
 * @brief Remove a graph reference from its folder, including its graph data.
 *
 * Captures the reference, its location, and its graph data on first execute so
 * undo restores the graph exactly.
 */
class RemoveGraphCommand : public SceneCommand {
public:
  explicit RemoveGraphCommand(const std::string &graph_id);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  std::string graph_id_;
  std::string parent_folder_id_; // Empty = root
  size_t index_{0};
  nlohmann::json graph_ref_json_;
  nlohmann::json graph_data_;
  bool has_graph_data_{false};
  bool captured_{false};
};

// ============================================================================
// RenameGraphCommand
// ============================================================================

class RenameGraphCommand : public SceneCommand {
public:
  RenameGraphCommand(std::string graph_id, std::string new_name);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  std::string graph_id_;
  std::string new_name_;
  std::string old_name_;
};

// ============================================================================
// AddTimelineSegmentCommand
// ============================================================================

class AddTimelineSegmentCommand : public SceneCommand {
public:
  explicit AddTimelineSegmentCommand(TimelineSegment segment);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  TimelineSegment segment_;
  size_t index_{0}; // Index of the appended segment
};

// ============================================================================
// RemoveTimelineSegmentCommand
// ============================================================================

class RemoveTimelineSegmentCommand : public SceneCommand {
public:
  explicit RemoveTimelineSegmentCommand(size_t index);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  size_t index_;
  TimelineSegment segment_;
};

// ============================================================================
// ImportAudioCommand
// ============================================================================

/**
 * @brief Import an audio track plus a timeline segment for it.
 *
 * The track may be moved into the scene (first execute) or rebuilt from its
 * path (redo, when the player has been moved out). Undo removes both the
 * segment and the track.
 */
class ImportAudioCommand : public SceneCommand {
public:
  explicit ImportAudioCommand(AudioTrack track);

  bool execute(Scene &scene) override;

  bool undo(Scene &scene) override;

  [[nodiscard]] std::string description() const override;

private:
  AudioTrack track_;
  std::string path_; // Rebuild source for redo
  double start_seconds_{0.0};
  std::string title_;
  size_t track_index_{0};
  size_t segment_index_{0};
};

#endif // NAG_EDITOR_SCENE_COMMANDS_H