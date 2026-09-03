#ifndef NAG_EDITOR_TIMELINE_CONTROLLER_H
#define NAG_EDITOR_TIMELINE_CONTROLLER_H

#include <cstdint>

#include "ImSequencer.h"
#include "editor/scene.h"

/**
 * @brief Adapter between the ImSequencer widget and the Scene timeline model.
 *
 * Implements ImSequencer::SequenceInterface by reading from and mutating a
 * Scene's timeline. All operations delegate to the Scene CRUD methods.
 */
class TimelineController : public ImSequencer::SequenceInterface {
public:
  explicit TimelineController(Scene &scene) : scene_(scene) {}

  [[nodiscard]] int GetFrameMin() const override { return 0; }
  [[nodiscard]] int GetFrameMax() const override { return scene_.total_frames; }
  [[nodiscard]] int GetItemCount() const override {
    return static_cast<int>(scene_.timeline.size());
  }
  [[nodiscard]] int GetItemTypeCount() const override { return 2; }
  [[nodiscard]] const char *GetItemTypeName(const int type_index) const override {
    return type_index == 0 ? "Graph" : "Audio";
  }
  [[nodiscard]] const char *GetItemLabel(const int index) const override;

  void Get(int index, int **start, int **end, int *type, unsigned int *color) override;

  void Add(int type) override;

  void Del(int index) override;

  void Duplicate(int index) override;

private:
  Scene &scene_;
};

#endif // NAG_EDITOR_TIMELINE_CONTROLLER_H
