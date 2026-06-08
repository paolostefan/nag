#ifndef NAG_ENGINE_AUDIO_TRACK_H
#define NAG_ENGINE_AUDIO_TRACK_H

#include <filesystem>
#include <memory>
#include <string>

#include "nlohmann/json.hpp"

#include "engine/i_audio_player.h"
#include "engine/mp3_player.h"
#include "engine/openmpt_player.h"

struct AudioTrack
{
  std::string title;
  std::filesystem::path path;
  std::unique_ptr<IAudioPlayer> player;
  double start_seconds = 0.0;
  bool track_is_module;


  AudioTrack() = default;

  AudioTrack(const std::filesystem::path &path,
             double start_seconds = 0.0)
      : path(path), start_seconds(start_seconds)
  {
    // TODO use a nicer way to determine the track title
    title = path.stem().string();
    const std::string extension = path.extension().string();
    if (extension == ".mod" || extension == ".xm")
    {
      player = std::make_unique<OpenMptPlayer>();
      track_is_module = true;
    }
    else if (extension == ".mp3")
    {
      // MP3 player
      player = std::make_unique<Mp3Player>();
      track_is_module = false;
    }
    else
      throw std::runtime_error("Unsupported audio format: " + extension);

    if (!player->load(path.string()))
    {
      throw std::runtime_error("Failed to load audio track: " + path.string());
    }
  }

  /// @brief Set the audio player associated with this track.
  /// @param p A unique pointer to an IAudioPlayer implementation.
  /// The ownership of the pointer is transferred to this object.
  void set_player(std::unique_ptr<IAudioPlayer> p) noexcept
  {
    player = std::move(p);
  }
};

inline void to_json(nlohmann::json &j, const AudioTrack &track)
{
  j = nlohmann::json{
      {"path", track.path.string()},
      {"start_seconds", track.start_seconds},
  };
}

inline void from_json(const nlohmann::json &j, AudioTrack &track)
{
  track = AudioTrack(
      j.at("path").get<std::filesystem::path>(),
      j.at("start_seconds").get<double>());
}

#endif // NAG_ENGINE_AUDIO_TRACK_H