#ifndef NAG_ENGINE_AUDIO_TRACK_H
#define NAG_ENGINE_AUDIO_TRACK_H

#include <filesystem>
#include <string>

struct AudioTrack
{
  std::string name;
  std::filesystem::path path;
  double start_seconds = 0.0;
};

#endif // NAG_ENGINE_AUDIO_TRACK_H