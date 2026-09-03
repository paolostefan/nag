#include <atomic>

#include "gtest/gtest.h"

#include "editor/audio_playback_controller.h"

namespace {

  class MockAudioPlayer : public IAudioPlayer {
  public:
    bool load(const std::string &) override { return true; }

    void play() override {
      call_count++;
      playback_state.store(PlaybackState::PLAYING);
      last_action = "play";
    }

    void pause() override {
      call_count++;
      playback_state.store(PlaybackState::PAUSED);
      last_action = "pause";
    }

    void stop() override {
      call_count++;
      playback_state.store(PlaybackState::STOPPED);
      last_action = "stop";
    }

    void seek(const double position_ms) override {
      call_count++;
      last_seek_ms = position_ms;
    }

    double get_position_ms() const override { return 0.0; }
    double get_duration_ms() const override { return 0.0; }

    PlaybackState get_playback_state() const override {
      return playback_state.load();
    }

    int call_count{0};
    double last_seek_ms{0.0};
    std::string last_action;
  };

  AudioTrack make_track() {
    AudioTrack track;
    track.set_player(std::make_unique<MockAudioPlayer>());
    return track;
  }

} // namespace

using audio_playback::AudioSegment;
using audio_playback::sync;

TEST(AudioPlaybackTest, PlaysTrackInsideActiveSegment) {
  std::vector<AudioTrack> tracks;
  tracks.push_back(make_track());
  auto *player = static_cast<MockAudioPlayer *>(tracks[0].player.get());

  // Segment covers frames 0..100 at 60fps (0s..~1.67s)
  const std::vector<AudioSegment> segments{{0, 0, 100}};

  sync(tracks, segments, 60, 30, /*flowing=*/true);

  EXPECT_EQ(player->last_action, "play");
  EXPECT_GT(player->last_seek_ms, 0.0);
  EXPECT_EQ(player->get_playback_state(), PlaybackState::PLAYING);
}

TEST(AudioPlaybackTest, DoesNotPlayWhenNotFlowing) {
  std::vector<AudioTrack> tracks;
  tracks.push_back(make_track());
  auto *player = static_cast<MockAudioPlayer *>(tracks[0].player.get());

  const std::vector<AudioSegment> segments{{0, 0, 100}};

  sync(tracks, segments, 60, 30, /*flowing=*/false);

  EXPECT_EQ(player->last_action, "");
  EXPECT_EQ(player->get_playback_state(), PlaybackState::STOPPED);
}

TEST(AudioPlaybackTest, PausesWhenOutsideSegment) {
  std::vector<AudioTrack> tracks;
  tracks.push_back(make_track());
  auto *player = static_cast<MockAudioPlayer *>(tracks[0].player.get());

  // Segment covers 0..100; current frame 200 is outside
  const std::vector<AudioSegment> segments{{0, 0, 100}};

  // Start playing by running inside the segment first
  sync(tracks, segments, 60, 50, true);
  EXPECT_EQ(player->get_playback_state(), PlaybackState::PLAYING);

  // Then move outside
  sync(tracks, segments, 60, 200, true);
  EXPECT_EQ(player->get_playback_state(), PlaybackState::PAUSED);
}

TEST(AudioPlaybackTest, IgnoresNegativeTrackIndex) {
  std::vector<AudioTrack> tracks{};
  const std::vector<AudioSegment> segments{{-1, 0, 100}};

  // Should not crash
  sync(tracks, segments, 60, 30, true);
  EXPECT_TRUE(true);
}

TEST(AudioPlaybackTest, IgnoresOutOfRangeTrackIndex) {
  std::vector<AudioTrack> tracks;
  tracks.push_back(make_track());
  auto *player = static_cast<MockAudioPlayer *>(tracks[0].player.get());

  const std::vector<AudioSegment> segments{{5, 0, 100}}; // index 5 > list size 1

  sync(tracks, segments, 60, 30, true);

  EXPECT_EQ(player->last_action, "");
}

TEST(AudioPlaybackTest, SeekUsesTrackOffset) {
  std::vector<AudioTrack> tracks;
  tracks.push_back(make_track());
  tracks[0].start_seconds = 10.0;
  auto *player = static_cast<MockAudioPlayer *>(tracks[0].player.get());

  // Segment 0..60 at 60fps. Frame 30 => 0.5s into segment.
  const std::vector<AudioSegment> segments{{0, 0, 60}};

  sync(tracks, segments, 60, 30, true);

  // offset = (0.5s - 0s) + 10s = 10.5s => 10500ms
  EXPECT_DOUBLE_EQ(player->last_seek_ms, 10500.0);
}
