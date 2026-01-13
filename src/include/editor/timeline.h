#ifndef NAG_EDITOR_TIMELINE_H
#define NAG_EDITOR_TIMELINE_H

#include <cstdio>
#include <cstdint>
#include <vector>

#include "imgui.h"
#include "ImSequencer.h"

enum class TrackType : uint8_t
{
  AUDIO,
  EFFECT
};

static const char *const kTrackTypeNames[] = {
    "Audio",
    "Effect"};

struct EditorTimeline : public ImSequencer::SequenceInterface
{
  int GetFrameMin() const override { return 0; }
  int GetFrameMax() const override { return 1000; }
  int GetItemCount() const override { return static_cast<int>(tracks.size()); }

  int GetItemTypeCount() const override { return 2; }

  const char *GetItemTypeName(int typeIndex) const override { return kTrackTypeNames[typeIndex]; }

  void Get(int index, int **start, int **end, int *type, unsigned int *color) override
  {
    if (index < 0 || index >= static_cast<int>(tracks.size()))
    {
      // todo: maybe throw?
      return;
    }

    const TimelineTrack &track = tracks[index];
    if (start)
      *start = const_cast<int *>(&track.frameStart);

    if (end)
      *end = const_cast<int *>(&track.frameEnd);

    if (type)
      *type = static_cast<int>(track.type);

    if (color)
      *color = track.type == TrackType::AUDIO ? 0xFF80A0FF : 0xFFFF8080;
  }

  void Add(int type) override
  {
    tracks.push_back(TimelineTrack{
        .type = static_cast<TrackType>(type),
        .frameStart = 0,
        .frameEnd = 100,
        .expanded = false});
  }

  constexpr void Add(TrackType t) { Add(static_cast<int>(t)); }

  void Del(int index) override { tracks.erase(tracks.begin() + index); }

  void Duplicate(int index) override
  {
    const TimelineTrack &track = tracks[index];
    tracks.push_back(TimelineTrack{
        .type = track.type,
        .frameStart = track.frameStart,
        .frameEnd = track.frameEnd,
        .expanded = false});
  }

  struct TimelineTrack
  {
    TrackType type;
    int frameStart;
    int frameEnd;
    bool expanded;
  };

  std::vector<TimelineTrack> tracks{};
};

#endif // NAG_EDITOR_TIMELINE_H