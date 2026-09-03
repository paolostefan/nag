#include "editor/audio_playback_controller.h"

namespace audio_playback {

void sync(
  const std::vector<AudioTrack> &audio_tracks,
  const std::vector<AudioSegment> &segments,
  const int fps,
  const int current_frame,
  const bool flowing
) {
  const double current_time_s = static_cast<double>(current_frame) / fps;

  for (const auto &seg: segments) {
    if (seg.audio_track_index < 0) continue;

    if (seg.audio_track_index >= static_cast<int>(audio_tracks.size())) continue;
    const AudioTrack &track = audio_tracks[static_cast<size_t>(seg.audio_track_index)];
    if (!track.player) continue;

    const double seg_start_s = static_cast<double>(seg.frame_start) / fps;
    const double seg_end_s = static_cast<double>(seg.frame_end) / fps;
    const bool in_range = flowing && current_time_s >= seg_start_s && current_time_s < seg_end_s;

    if (in_range) {
      const double track_offset_s = current_time_s - seg_start_s + track.start_seconds;
      if (track.player->get_playback_state() != PlaybackState::PLAYING) {
        track.player->play();
      }
      track.player->seek(track_offset_s * 1000.0);
    } else {
      if (track.player->get_playback_state() == PlaybackState::PLAYING) {
        track.player->pause();
      }
    }
  }
}

} // namespace audio_playback
