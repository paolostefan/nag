#ifndef NAG_ENGINE_MP3_PLAYER_H
#define NAG_ENGINE_MP3_PLAYER_H

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

#include "mpg123.h"
#include "SDL2/SDL.h"

#include "engine/i_audio_player.h"

class Mp3Player : public IAudioPlayer
{
public:
  explicit Mp3Player();
  ~Mp3Player();

  bool load(const std::string &path) override;

  void play() override;
  void pause() override;
  void stop() override;
  void seek(const double position_ms) override;

  double get_position_ms() const override;
  double get_duration_ms() const override;

  PlaybackState get_playback_state() const override;

private:
  void decode(float *out, int frames);
  static void audio_callback(void *userdata, Uint8 *stream, int len);

private:
  mutable std::mutex decode_mx;
  std::atomic<PlaybackState> playback_state{PlaybackState::STOPPED};

  struct Mpg123Deleter
  {
    void operator()(mpg123_handle *h) const
    {
      if (h)
        mpg123_delete(h);
    }
  };

  std::unique_ptr<mpg123_handle, Mpg123Deleter> mh{};

  long samplerate = 44100;
  int channels = 2;

  double position = 0.0;
  double duration = 0.0;
};

#endif // NAG_ENGINE_MP3_PLAYER_H