#include "engine/mp3_player.h"

#include <cstring>

#include "spdlog/spdlog.h"

double Mp3Player::get_position_ms() const
{
  const off_t sample_offset = mpg123_tell(mh.get());
  return (static_cast<double>(sample_offset) / static_cast<double>(samplerate)) * 1000.0;
}

double Mp3Player::get_duration_ms() const
{
  const off_t total_samples = mpg123_length(mh.get());
  return (static_cast<double>(total_samples) / static_cast<double>(samplerate)) * 1000.0;
}

PlaybackState Mp3Player::get_playback_state() const
{
  return playback_state.load();
}

Mp3Player::Mp3Player()
{
}

Mp3Player::~Mp3Player()
{
  stop();

  if (device)
    SDL_CloseAudioDevice(device);

  if (mh)
    mpg123_close(mh.get());

  // mpg123_delete() is done in the custom unique_ptr deleter
}

bool Mp3Player::load(const std::string &path)
{
  mh.reset(mpg123_new(nullptr, nullptr));

  if (!mh)
  {
    spdlog::error("mpg123_new failed");
    return false;
  }

  mpg123_open(mh.get(), path.c_str());
  mpg123_getformat(mh.get(), &samplerate, &channels, nullptr);

  mpg123_format_none(mh.get());
  mpg123_format(mh.get(), samplerate, channels, MPG123_ENC_SIGNED_16);

  SDL_AudioSpec want{};
  want.freq = samplerate;
  want.format = AUDIO_F32SYS;
  want.channels = static_cast<Uint8>(channels);
  want.samples = 4096;
  want.callback = Mp3Player::audio_callback;
  want.userdata = this;

  device = SDL_OpenAudioDevice(nullptr, 0, &want, &obtained, 0);
  if (!device)
  {
    spdlog::error("SDL_OpenAudioDevice failed");
    return false;
  }

  return true;
}

void Mp3Player::play()
{
  if (playback_state == PlaybackState::PLAYING)
    return;

  playback_state = PlaybackState::PLAYING;
  SDL_PauseAudioDevice(device, 0);
}

void Mp3Player::pause()
{
  if (playback_state != PlaybackState::PLAYING)
    return;

  playback_state = PlaybackState::PAUSED;
  SDL_PauseAudioDevice(device, 1);
}

void Mp3Player::stop()
{
  playback_state = PlaybackState::STOPPED;
  SDL_PauseAudioDevice(device, 1);
  mpg123_seek(mh.get(), 0, SEEK_SET);
}

void Mp3Player::seek(const double position_ms)
{
  const off_t sample_offset = static_cast<off_t>(
      (position_ms / 1000.0) * static_cast<double>(samplerate));
  mpg123_seek(mh.get(), sample_offset, SEEK_SET);
}

void Mp3Player::audio_callback(void *userdata, Uint8 *stream, int len)
{
  auto *self = static_cast<Mp3Player *>(userdata);

  if (self->get_playback_state() != PlaybackState::PLAYING)
    return;

  std::memset(stream, 0, len);
  const int frames = len / (sizeof(float) * self->channels);
  self->decode(reinterpret_cast<float *>(stream), frames);
}

void Mp3Player::decode(float *out, int frames)
{
  std::lock_guard lock(decode_mx);

  std::vector<int16_t> pcm(frames * channels);
  size_t done = 0;

  int err = mpg123_read(
      mh.get(),
      reinterpret_cast<unsigned char *>(pcm.data()),
      pcm.size() * sizeof(int16_t),
      &done);

  if (err == MPG123_DONE || done == 0)
  {
    playback_state = PlaybackState::STOPPED;
    return;
  }

  const int samples = done / sizeof(int16_t);
  for (int i = 0; i < samples; ++i)
    out[i] = pcm[i] / 32768.f;
}
