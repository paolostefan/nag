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

NLOHMANN_JSON_SERIALIZE_ENUM(TrackType, {
  {TrackType::AUDIO, "Audio"},
  {TrackType::EFFECT, "Effect"},
})

struct TimelineTrack
{
  TrackType type;
  int frameStart;
  int frameEnd;
  bool expanded;

  union {
    size_t audio_track_index;
    size_t effect_id;
  };
};

inline void to_json(nlohmann::json &j, const TimelineTrack &track)
{
  j = nlohmann::json{
      {"type", track.type},
      {"frameStart", track.frameStart},
      {"frameEnd", track.frameEnd},
      {"expanded", track.expanded}};

  if (track.type == TrackType::AUDIO)
    j["audio_track_index"] = track.audio_track_index;
  else
    j["effect_id"] = track.effect_id;
}

inline void from_json(const nlohmann::json &j, TimelineTrack &track)
{
  j.at("type").get_to(track.type);
  j.at("frameStart").get_to(track.frameStart);
  j.at("frameEnd").get_to(track.frameEnd);
  j.at("expanded").get_to(track.expanded);

  if (track.type == TrackType::AUDIO)
    j.at("audio_track_index").get_to(track.audio_track_index);
  else
    j.at("effect_id").get_to(track.effect_id);
}

struct Timeline : public ImSequencer::SequenceInterface
{
  [[nodiscard]] int GetFrameMin() const override { return 0; }
  [[nodiscard]] int GetFrameMax() const override { return 1000; }
  [[nodiscard]] int GetItemCount() const override { return static_cast<int>(tracks.size()); }

  [[nodiscard]] int GetItemTypeCount() const override { return 2; }

  [[nodiscard]] const char *GetItemTypeName(int typeIndex) const override { return kTrackTypeNames[typeIndex]; }

  void Get(const int index, int **start, int **end, int *type, unsigned int *color) override
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
      *color = track.type == TrackType::AUDIO ? 0xFF80A0FF : 0xFF80FF80;
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

  void Del(const int index) override { tracks.erase(tracks.begin() + index); }

  void Duplicate(int index) override
  {
    const TimelineTrack &track = tracks[index];
    tracks.push_back(TimelineTrack{
        .type = track.type,
        .frameStart = track.frameStart,
        .frameEnd = track.frameEnd,
        .expanded = false});
  }

  std::vector<TimelineTrack> tracks{};
};

inline void to_json(nlohmann::json &j, const Timeline &timeline)
{
  j = nlohmann::json{
      {"tracks", timeline.tracks}};
}

inline void from_json(const nlohmann::json &j, Timeline &timeline)
{
  j.at("tracks").get_to(timeline.tracks);
}

#endif // NAG_EDITOR_TIMELINE_H