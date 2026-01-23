#include "gtest/gtest.h"

#include "editor/timeline.h"

class TimelineTest : public ::testing::Test {
};

// Timeline Tests
TEST_F(TimelineTest, TimelineDefaultConstruction)
{
  const Timeline timeline;

  EXPECT_EQ(timeline.GetFrameMin(), 0);
  EXPECT_EQ(timeline.GetFrameMax(), 1000);
  EXPECT_EQ(timeline.GetItemCount(), 0);
  EXPECT_EQ(timeline.GetItemTypeCount(), 2);
  EXPECT_TRUE(timeline.tracks.empty());
}

TEST_F(TimelineTest, TimelineGetItemTypeName)
{
  Timeline timeline;

  EXPECT_STREQ(timeline.GetItemTypeName(0), "Audio");
  EXPECT_STREQ(timeline.GetItemTypeName(1), "Effect");
}

TEST_F(TimelineTest, TimelineAddAudioTrack)
{
  Timeline timeline;

  timeline.Add(static_cast<int>(TrackType::AUDIO));

  EXPECT_EQ(timeline.GetItemCount(), 1);
  EXPECT_EQ(timeline.tracks[0].type, TrackType::AUDIO);
  EXPECT_EQ(timeline.tracks[0].frameStart, 0);
  EXPECT_EQ(timeline.tracks[0].frameEnd, 100);
  EXPECT_FALSE(timeline.tracks[0].expanded);
}

TEST_F(TimelineTest, TimelineAddEffectTrack)
{
  Timeline timeline;

  timeline.Add(TrackType::EFFECT);

  EXPECT_EQ(timeline.GetItemCount(), 1);
  EXPECT_EQ(timeline.tracks[0].type, TrackType::EFFECT);
  EXPECT_EQ(timeline.tracks[0].frameStart, 0);
  EXPECT_EQ(timeline.tracks[0].frameEnd, 100);
  EXPECT_FALSE(timeline.tracks[0].expanded);
}

TEST_F(TimelineTest, TimelineAddMultipleTracks)
{
  Timeline timeline;

  timeline.Add(TrackType::AUDIO);
  timeline.Add(TrackType::EFFECT);
  timeline.Add(TrackType::AUDIO);

  EXPECT_EQ(timeline.GetItemCount(), 3);
  EXPECT_EQ(timeline.tracks[0].type, TrackType::AUDIO);
  EXPECT_EQ(timeline.tracks[1].type, TrackType::EFFECT);
  EXPECT_EQ(timeline.tracks[2].type, TrackType::AUDIO);
}

TEST_F(TimelineTest, TimelineDelTrack)
{
  Timeline timeline;

  timeline.Add(TrackType::AUDIO);
  timeline.Add(TrackType::EFFECT);
  timeline.Add(TrackType::AUDIO);

  EXPECT_EQ(timeline.GetItemCount(), 3);

  timeline.Del(1);  // Remove the effect track

  EXPECT_EQ(timeline.GetItemCount(), 2);
  EXPECT_EQ(timeline.tracks[0].type, TrackType::AUDIO);
  EXPECT_EQ(timeline.tracks[1].type, TrackType::AUDIO);
}

TEST_F(TimelineTest, TimelineDuplicateTrack)
{
  Timeline timeline;

  timeline.Add(TrackType::AUDIO);
  timeline.tracks[0].frameStart = 50;
  timeline.tracks[0].frameEnd = 200;
  timeline.tracks[0].expanded = true;
  timeline.tracks[0].audio_track_index = 3;

  timeline.Duplicate(0);

  EXPECT_EQ(timeline.GetItemCount(), 2);
  EXPECT_EQ(timeline.tracks[1].type, TrackType::AUDIO);
  EXPECT_EQ(timeline.tracks[1].frameStart, 50);
  EXPECT_EQ(timeline.tracks[1].frameEnd, 200);
  EXPECT_FALSE(timeline.tracks[1].expanded);  // Duplicate sets expanded to false
  EXPECT_EQ(timeline.tracks[1].audio_track_index, 3);
}

TEST_F(TimelineTest, TimelineGetMethod)
{
  Timeline timeline;

  timeline.Add(TrackType::AUDIO);
  timeline.tracks[0].frameStart = 25;
  timeline.tracks[0].frameEnd = 150;

  int *start = nullptr;
  int *end = nullptr;
  int type = -1;
  unsigned int color = 0;

  timeline.Get(0, &start, &end, &type, &color);

  ASSERT_NE(start, nullptr);
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(*start, 25);
  EXPECT_EQ(*end, 150);
  EXPECT_EQ(type, static_cast<int>(TrackType::AUDIO));
  EXPECT_EQ(color, 0xFF80A0FF);  // Audio color
}

TEST_F(TimelineTest, TimelineGetMethodEffectColor)
{
  Timeline timeline;

  timeline.Add(TrackType::EFFECT);

  int type = -1;
  unsigned int color = 0;

  timeline.Get(0, nullptr, nullptr, &type, &color);

  EXPECT_EQ(type, static_cast<int>(TrackType::EFFECT));
  EXPECT_EQ(color, 0xFF80FF80);  // Effect color
}

TEST_F(TimelineTest, TimelineGetMethodInvalidIndex)
{
  Timeline timeline;

  timeline.Add(TrackType::AUDIO);

  int *start = nullptr;
  int *end = nullptr;
  int type = -1;
  unsigned int color = 0;

  // Test with a negative index
  timeline.Get(-1, &start, &end, &type, &color);
  EXPECT_EQ(start, nullptr);
  EXPECT_EQ(end, nullptr);

  // Test with an out-of-bounds index
  timeline.Get(5, &start, &end, &type, &color);
  EXPECT_EQ(start, nullptr);
  EXPECT_EQ(end, nullptr);
}

TEST_F(TimelineTest, TimelineSerialization)
{
  Timeline timeline;

  timeline.Add(TrackType::AUDIO);
  timeline.tracks[0].frameStart = 10;
  timeline.tracks[0].frameEnd = 50;
  timeline.tracks[0].expanded = true;
  timeline.tracks[0].audio_track_index = 2;

  timeline.Add(TrackType::EFFECT);
  timeline.tracks[1].frameStart = 60;
  timeline.tracks[1].frameEnd = 120;
  timeline.tracks[1].expanded = false;
  timeline.tracks[1].effect_id = 7;

  nlohmann::json j = timeline;

  EXPECT_TRUE(j.contains("tracks"));
  EXPECT_EQ(j["tracks"].size(), 2);
  EXPECT_EQ(j["tracks"][0]["type"], "Audio");
  EXPECT_EQ(j["tracks"][0]["frameStart"], 10);
  EXPECT_EQ(j["tracks"][0]["frameEnd"], 50);
  EXPECT_EQ(j["tracks"][0]["expanded"], true);
  EXPECT_EQ(j["tracks"][0]["audio_track_index"], 2);
  EXPECT_EQ(j["tracks"][1]["type"], "Effect");
  EXPECT_EQ(j["tracks"][1]["frameStart"], 60);
  EXPECT_EQ(j["tracks"][1]["frameEnd"], 120);
  EXPECT_EQ(j["tracks"][1]["expanded"], false);
  EXPECT_EQ(j["tracks"][1]["effect_id"], 7);
}

TEST_F(TimelineTest, TimelineDeserialization)
{
  nlohmann::json j = {
      {"tracks", {
          {
              {"type", "Audio"},
              {"frameStart", 15},
              {"frameEnd", 75},
              {"expanded", true},
              {"audio_track_index", 4}
          },
          {
              {"type", "Effect"},
              {"frameStart", 80},
              {"frameEnd", 140},
              {"expanded", false},
              {"effect_id", 9}
          }
      }}
  };

  Timeline timeline = j.get<Timeline>();

  EXPECT_EQ(timeline.GetItemCount(), 2);
  EXPECT_EQ(timeline.tracks[0].type, TrackType::AUDIO);
  EXPECT_EQ(timeline.tracks[0].frameStart, 15);
  EXPECT_EQ(timeline.tracks[0].frameEnd, 75);
  EXPECT_EQ(timeline.tracks[0].expanded, true);
  EXPECT_EQ(timeline.tracks[0].audio_track_index, 4);
  EXPECT_EQ(timeline.tracks[1].type, TrackType::EFFECT);
  EXPECT_EQ(timeline.tracks[1].frameStart, 80);
  EXPECT_EQ(timeline.tracks[1].frameEnd, 140);
  EXPECT_EQ(timeline.tracks[1].expanded, false);
  EXPECT_EQ(timeline.tracks[1].effect_id, 9);
}

TEST_F(TimelineTest, TimelineEmptySerialization)
{
  Timeline timeline;

  nlohmann::json j = timeline;

  EXPECT_TRUE(j.contains("tracks"));
  EXPECT_EQ(j["tracks"].size(), 0);
}

TEST_F(TimelineTest, TimelineEmptyDeserialization)
{
  nlohmann::json j = {{"tracks", nlohmann::json::array()}};

  Timeline timeline = j.get<Timeline>();

  EXPECT_EQ(timeline.GetItemCount(), 0);
  EXPECT_TRUE(timeline.tracks.empty());
}
