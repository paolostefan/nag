#ifndef NAG_EDITOR_AUDIO_PLAYBACK_CONTROLLER_H
#define NAG_EDITOR_AUDIO_PLAYBACK_CONTROLLER_H

#include <cstdint>
#include <vector>

#include "engine/audio_track.h"

/**
 * @brief Pure playback logic for audio segments on the timeline.
 *
 * Decoupled from the editor UI. The caller supplies the audio tracks,
 * the list of audio segments, and the current playback position.
 */
namespace audio_playback {

  /// @brief A timeline segment referring to an audio track by index.
  struct AudioSegment {
    int audio_track_index{-1};
    int frame_start{0};
    int frame_end{100};
  };

  /**
   * @brief Advance (or pause) audio playback to match the current frame.
   *
   * For every audio segment active at @p current_frame (when @p flowing),
   * the referenced track is played and seeked to the correct offset.
   * Segments outside the active range are paused.
   *
   * @param audio_tracks  The scene's audio tracks (indexed by segment.audio_track_index).
   * @param segments      The audio segments on the timeline.
   * @param fps           Scene frames per second.
   * @param current_frame Current playback head position.
   * @param flowing       True when the timeline is playing (not paused).
   */
  void sync(
    const std::vector<AudioTrack> &audio_tracks,
    const std::vector<AudioSegment> &segments,
    int fps,
    int current_frame,
    bool flowing
  );

} // namespace audio_playback

#endif // NAG_EDITOR_AUDIO_PLAYBACK_CONTROLLER_H
