#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include "editor/timeline.h"

class TimelineJsonTest : public testing::Test
{
protected:
  void SetUp() override
  {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override
  {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

// TimelineTrack Tests
TEST_F(TimelineJsonTest, TimelineTrackEnumSerialization)
{
  const nlohmann::json j_audio = TrackType::AUDIO;
  EXPECT_EQ(j_audio, "Audio");

  const nlohmann::json j_effect = TrackType::EFFECT;
  EXPECT_EQ(j_effect, "Effect");
}

TEST_F(TimelineJsonTest, TimelineTrackEnumDeserialization)
{
  const nlohmann::json j_audio = "Audio";
  TrackType type_audio = j_audio.get<TrackType>();
  EXPECT_EQ(type_audio, TrackType::AUDIO);

  const nlohmann::json j_effect = "Effect";
  TrackType type_effect = j_effect.get<TrackType>();
  EXPECT_EQ(type_effect, TrackType::EFFECT);
}

TEST_F(TimelineJsonTest, TimelineTrackSerialization)
{
  TimelineTrack track{};
  track.type = TrackType::AUDIO;
  track.frameStart = 10;
  track.frameEnd = 50;
  track.expanded = true;
  track.audio_track_index = 2;

  nlohmann::json j = track;

  EXPECT_EQ(j["type"], "Audio");
  EXPECT_EQ(j["frameStart"], 10);
  EXPECT_EQ(j["frameEnd"], 50);
  EXPECT_EQ(j["expanded"], true);
  EXPECT_EQ(j["audio_track_index"], 2);
}

// TimelineTrack Deserialization Test
TEST_F(TimelineJsonTest, TimelineTrackDeserialization)
{
  nlohmann::json j = {
      {"type", "Effect"},
      {"frameStart", 20},
      {"frameEnd", 80},
      {"expanded", false},
      {"effect_id", 5}};  // Effect ID

  const TimelineTrack track = j.get<TimelineTrack>();

  EXPECT_EQ(track.type, TrackType::EFFECT);
  EXPECT_EQ(track.frameStart, 20);
  EXPECT_EQ(track.frameEnd, 80);
  EXPECT_EQ(track.expanded, false);
  EXPECT_EQ(track.effect_id, 5);
}
