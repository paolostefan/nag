#ifndef NAG_ENGINE_OPENMPT_PLAYER_H
#define NAG_ENGINE_OPENMPT_PLAYER_H

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

#include "libopenmpt/libopenmpt.hpp"
#include "SDL2/SDL.h"

#include "i_audio_player.h"

class OpenMptPlayer : public IAudioPlayer
{
  std::unique_ptr<openmpt::module> mod{};
  double samplerate = 44100.0;
  double position = 0.0;
  double duration = 0.0;
  SDL_AudioDeviceID device{};

  std::atomic<PlaybackState> playback_state{PlaybackState::STOPPED};

  std::atomic<bool> seek_requested{false};
  std::atomic<double> seek_position{0.0};

  mutable std::mutex mod_mx;

public:
  double get_position_ms() const override
  {
    std::lock_guard<std::mutex> lock(mod_mx);
    return mod ? mod->get_position_seconds() * 1000.0 : 0.0;
  }

  double get_duration_ms() const override
  {
    return duration;
  }

  double get_bpm() const override
  {
    std::lock_guard<std::mutex> lock(mod_mx);
    return mod ? mod->get_current_speed() * (mod->get_current_tempo2() / 24.0) : 0.0;
  }

  int get_row() const override
  {
    std::lock_guard<std::mutex> lock(mod_mx);
    return mod ? mod->get_current_row() : -1;
  }

  int get_pattern() const override
  {
    std::lock_guard<std::mutex> lock(mod_mx);
    return mod ? mod->get_current_pattern() : -1;
  }

  bool load(const std::string &path) override
  {
    if (mod)
    {
      stop();
      mod.reset();
    }

    std::lock_guard<std::mutex> lock(mod_mx);

    audio_path = path;

    std::ifstream in_file_stream(audio_path, std::ios::binary);
    if (!in_file_stream.is_open())
      return false;

    std::vector<char> data((std::istreambuf_iterator<char>(in_file_stream)), {});
    mod = std::make_unique<openmpt::module>(data);

    position = 0.0;
    duration = mod->get_duration_seconds() * 1000.0;
    playback_state = PlaybackState::STOPPED;

    return true;
  }

  PlaybackState get_playback_state() const override
  {
    std::lock_guard<std::mutex> lock(mod_mx);
    return playback_state;
  }

  void play() override
  {
    std::lock_guard<std::mutex> lock(mod_mx);

    if (!mod)
    {
      return;
    }

    if (playback_state == PlaybackState::PAUSED)
    {
      // Resume playback
      SDL_PauseAudioDevice(device, 0);
      playback_state = PlaybackState::PLAYING;
      return;
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);

    spec.freq = samplerate;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 1024;
    spec.callback = audio_callback;
    spec.userdata = this;

    if (device)
      SDL_CloseAudioDevice(device);

    device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    if (!device)
      return;

    playback_state = PlaybackState::PLAYING;
    SDL_PauseAudioDevice(device, 0);
  }

  void pause() override
  {
    if (playback_state == PlaybackState::PLAYING && device)
    {
      SDL_PauseAudioDevice(device, 1);
      playback_state = PlaybackState::PAUSED;
    }
  }

  void stop() override
  {
    std::lock_guard<std::mutex> lock(mod_mx);

    if (device)
    {
      SDL_CloseAudioDevice(device);
      device = 0;
    }

    playback_state = PlaybackState::STOPPED;
    position = 0.0;

    if (mod)
      mod->set_position_seconds(0.0);
  }

  void seek(const double position_ms) override
  {
    if (!mod || position_ms < 0.0 || position_ms > duration)
      return;

    seek_position = position_ms;
    seek_requested = true;
  }

private:
  static void audio_callback(void *userdata, Uint8 *stream, int len)
  {
    OpenMptPlayer *self = static_cast<OpenMptPlayer *>(userdata);

    // Handle seek requests
    if (self->seek_requested.load())
    {
      std::lock_guard<std::mutex> lock(self->mod_mx);
      if (self->mod)
      {
        self->mod->set_position_seconds(self->seek_position.load());
        self->seek_requested = false;
      }
    }

    std::lock_guard<std::mutex> lock(self->mod_mx);
    if (!self->mod || self->playback_state.load() != PlaybackState::PLAYING)
    {
      memset(stream, 0, len);
      return;
    }

    int frames = len / (sizeof(float) * 2);
    std::vector<float> buffer(frames * 2);

    int count = self->mod->read_interleaved_stereo(self->samplerate, frames, buffer.data());

    if (count > 0)
    {
      memcpy(stream, buffer.data(), count * 2 * sizeof(float));
      if (self->mod->get_position_seconds() >= self->mod->get_duration_seconds())
      {
        self->playback_state = PlaybackState::STOPPED;
      }
    }
    else
    {
      memset(stream, 0, len); // fine eeee...ilmodulo!
    }
  }
};

#endif // NAG_ENGINE_OPENMPT_PLAYER_H