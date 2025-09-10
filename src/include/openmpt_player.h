#ifndef NAG_ENGINE_OPENMPT_PLAYER_H
#define NAG_ENGINE_OPENMPT_PLAYER_H

#include <memory>
#include <fstream>
#include <iostream>

#include <SDL2/SDL.h>
#include <libopenmpt/libopenmpt.hpp>

#include "i_audio_player.h"

class OpenMptPlayer : public IAudioPlayer
{
  std::unique_ptr<openmpt::module> mod{};
  double samplerate = 44100.0;
  double position = 0.0;
  SDL_AudioDeviceID device{};

public:
  bool load(const std::string &path) override
  {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
      return false;

    std::vector<char> data((std::istreambuf_iterator<char>(file)), {});
    mod = std::make_unique<openmpt::module>(data);
    return true;
  }

  void play() override
  {
    if (!mod)
      return;
    position = 0.0;

    SDL_AudioSpec spec;
    SDL_zero(spec);

    spec.freq = samplerate;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 1024;
    spec.callback = audio_callback;
    spec.userdata = this;

    device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    if (!device)
      return;
    SDL_PauseAudioDevice(device, 0);
  }

  void stop() override
  {
    if (!device)
      return;

    SDL_CloseAudioDevice(device);
    device = 0;

    position = 0.0;

    if (mod)
      mod.reset();
  }

  double getPositionMs() const override
  {
    return mod ? mod->get_position_seconds() * 1000.0 : 0.0;
  }

  double getBpm() const override
  {
    return mod ? mod->get_current_speed() * (mod->get_current_tempo() / 24.0) : 0.0;
  }

  int getRow() const override
  {
    return mod ? mod->get_current_row() : -1;
  }

  int getPattern() const override
  {
    return mod ? mod->get_current_pattern() : -1;
  }

private:
  static void audio_callback(void *userdata, Uint8 *stream, int len)
  {
    OpenMptPlayer *self = static_cast<OpenMptPlayer *>(userdata);
    if (!self->mod)
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
    }
    else
    {
      memset(stream, 0, len); // fine modulo
    }
  }
};

#endif // NAG_ENGINE_OPENMPT_PLAYER_H