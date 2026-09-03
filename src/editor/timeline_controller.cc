#include "editor/timeline_controller.h"

const char *TimelineController::GetItemLabel(const int index) const {
  if (index < 0 || index >= static_cast<int>(scene_.timeline.size())) {
    return "";
  }
  const auto &seg = scene_.timeline[static_cast<size_t>(index)];
  if (seg.type == SegmentType::GRAPH) {
    const auto *graph = scene_.find_graph(seg.graph_id);
    return graph ? graph->name.c_str() : "(unknown graph)";
  }
  const auto *track = scene_.get_audio_track(static_cast<size_t>(seg.audio_track_index));
  return track ? track->title.c_str() : "(unknown audio)";
}

void TimelineController::Get(const int index, int **start, int **end, int *type, unsigned int *color) {
  if (index < 0 || index >= static_cast<int>(scene_.timeline.size())) {
    return;
  }

  const TimelineSegment &segment = scene_.timeline[static_cast<size_t>(index)];
  if (start)
    *start = const_cast<int *>(&segment.frame_start);
  if (end)
    *end = const_cast<int *>(&segment.frame_end);
  if (type)
    *type = static_cast<int>(segment.type);
  if (color)
    *color = segment.color;
}

void TimelineController::Add(const int type) {
  if (type == static_cast<int>(SegmentType::AUDIO)) {
    const TimelineSegment segment(0, 100, 0);
    scene_.add_timeline_segment(segment);
  } else {
    const TimelineSegment segment(0, 100, "");
    scene_.add_timeline_segment(segment);
  }
}

void TimelineController::Del(const int index) {
  if (index >= 0 && index < static_cast<int>(scene_.timeline.size())) {
    scene_.remove_timeline_segment(static_cast<size_t>(index));
  }
}

void TimelineController::Duplicate(const int index) {
  if (index >= 0 && index < static_cast<int>(scene_.timeline.size())) {
    const TimelineSegment &original = scene_.timeline[static_cast<size_t>(index)];
    TimelineSegment copy = original;
    copy.frame_start = original.frame_end;
    copy.frame_end = original.frame_end + (original.frame_end - original.frame_start);
    scene_.add_timeline_segment(copy);
  }
}
