#include "engine/openmpt_player.h"

double OpenMptPlayer::get_position_ms() const
{
  std::lock_guard<std::mutex> lock(mod_mx);
  return mod ? mod->get_position_seconds() * 1000.0 : 0.0;
}

double OpenMptPlayer::get_duration_ms() const
{
  return duration;
}

double OpenMptPlayer::get_bpm() const
{
  std::lock_guard<std::mutex> lock(mod_mx);
  return mod ? mod->get_current_speed() * (mod->get_current_tempo2() / 24.0) : 0.0;
}

int OpenMptPlayer::get_row() const
{
  std::lock_guard<std::mutex> lock(mod_mx);
  return mod ? mod->get_current_row() : -1;
}

int OpenMptPlayer::get_pattern() const
{
  std::lock_guard<std::mutex> lock(mod_mx);
  return mod ? mod->get_current_pattern() : -1;
}

bool OpenMptPlayer::load(const std::string &path)
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

PlaybackState OpenMptPlayer::get_playback_state() const
{
  std::lock_guard<std::mutex> lock(mod_mx);
  return playback_state;
}

void OpenMptPlayer::play()
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

void OpenMptPlayer::pause()
{
  if (playback_state == PlaybackState::PLAYING && device)
  {
    SDL_PauseAudioDevice(device, 1);
    playback_state = PlaybackState::PAUSED;
  }
}

void OpenMptPlayer::stop()
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

void OpenMptPlayer::seek(const double position_ms)
{
  if (!mod || position_ms < 0.0 || position_ms > duration)
    return;

  seek_position = position_ms;
  seek_requested = true;
}

void OpenMptPlayer::audio_callback(void *userdata, Uint8 *stream, int len)
{
  OpenMptPlayer *self = static_cast<OpenMptPlayer *>(userdata);

  // Handle seek requests
  if (self->seek_requested.load())
  {
    std::lock_guard<std::mutex> lock(self->mod_mx);
    if (self->mod)
    {
      self->mod->set_position_seconds(self->seek_position.load() / 1000.0);
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