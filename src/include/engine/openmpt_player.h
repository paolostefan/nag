#ifndef NAG_ENGINE_OPENMPT_PLAYER_H
#define NAG_ENGINE_OPENMPT_PLAYER_H

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

#include "libopenmpt/libopenmpt.hpp"
#include "SDL2/SDL.h"

#include "engine/i_audio_player.h"

class OpenMptPlayer : public IAudioPlayer
{
  std::unique_ptr<openmpt::module> mod{};
  double samplerate = 44100.0;
  double position = 0.0;
  double duration = 0.0;

  mutable std::mutex mod_mx;

public:
  double get_position_ms() const override;
  double get_duration_ms() const override;
  double get_bpm() const override;

  int get_row() const override;
  int get_pattern() const override;

  bool load(const std::string &path) override;

  PlaybackState get_playback_state() const override;

  void play() override;
  void pause() override;
  void stop() override;
  void seek(const double position_ms) override;

private:
  static void audio_callback(void *userdata, Uint8 *stream, int len);
};

#endif // NAG_ENGINE_OPENMPT_PLAYER_H