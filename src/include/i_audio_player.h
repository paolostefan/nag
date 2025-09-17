#ifndef NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H
#define NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H

#include <cstdint>
#include <filesystem>
#include <string>

enum class PlaybackState:uint8_t {
  STOPPED,
  PLAYING,
  PAUSED
};

class IAudioPlayer
{
public:
  virtual ~IAudioPlayer() = default;

  virtual bool load(const std::string &path) = 0;
  virtual void play() = 0;
  virtual void pause() = 0;
  virtual void stop() = 0;

  virtual double get_position_ms() const = 0;     // Current position in milliseconds
  virtual double get_bpm() const { return 0.0; } // BPM
  virtual int get_row() const { return -1; }     // Current row (only for modules)
  virtual int get_pattern() const { return -1; } // Current pattern (only for modules)
  virtual PlaybackState get_playback_state() const = 0;

  inline std::filesystem::path get_audio_path() const { return audio_path; }
  inline std::string get_audio_path_str() const { return audio_path.string(); }

protected:
  std::filesystem::path audio_path{};
};

#endif // NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H
