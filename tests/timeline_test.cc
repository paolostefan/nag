#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include "editor/timeline.h"

class TimelineTest : public ::testing::Test
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
TEST_F(TimelineTest, TimelineTrackEnumSerialization)
{
  nlohmann::json j_audio = TrackType::AUDIO;
  EXPECT_EQ(j_audio, "Audio");

  nlohmann::json j_effect = TrackType::EFFECT;
  EXPECT_EQ(j_effect, "Effect");
}