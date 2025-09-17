#ifndef NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H
#define NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H

#include <filesystem>
#include <string>

class IAudioPlayer
{
public:
  virtual ~IAudioPlayer() = default;

  virtual bool load(const std::string &path) = 0;
  virtual void play() = 0;
  virtual void stop() = 0;

  virtual double getPositionMs() const = 0;     // Current position in milliseconds
  virtual double getBpm() const { return 0.0; } // BPM
  virtual int getRow() const { return -1; }     // Current row (only for modules)
  virtual int getPattern() const { return -1; } // Current pattern (only for modules)

  inline std::filesystem::path get_audio_path() const { return audio_path; }
  inline std::string get_audio_path_str() const { return audio_path.string(); }

protected:
  std::filesystem::path audio_path{};
};

#endif // NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H
