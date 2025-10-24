#ifndef NAG_ENGINE_PROJECT_H
#define NAG_ENGINE_PROJECT_H

#include <filesystem>
#include <string>
#include <vector>

#include "audio_track.h"

class Project
{
public:
  Project() = default;

  Project(const std::string &name) : name(name) {}

  const std::string &get_name() const noexcept { return name; }
  const std::vector<AudioTrack> &get_audio_tracks() const noexcept
  {
    return audio_tracks;
  }

private:
  /// Path to the project descriptor file
  std::filesystem::path path{};

  /// Project name
  std::string name{"Void of space"};

  std::vector<AudioTrack> audio_tracks{};

  bool pristine{true};
};

#endif // NAG_ENGINE_PROJECT_H