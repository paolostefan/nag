#ifndef NAG_ENGINE_PROJECT_H
#define NAG_ENGINE_PROJECT_H

#include <filesystem>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "editor/timeline.h"
#include "engine/audio_track.h"

struct Project
{
  int current_frame{0};
  uint8_t fps{60};

  Timeline timeline{};

  /// Path to the project descriptor file
  std::filesystem::path path{};

  /// Project name
  std::string name{"Void of space"};

  std::vector<AudioTrack> audio_tracks{};

  bool pristine{true};

  Project() = default;
  Project(const std::string &name) : name(name) {}

  inline void add_audio_track(const std::string &path,
                              const double start_seconds = 0.0)
  {
    audio_tracks.emplace_back(path, start_seconds);
    pristine = false;
  }

  bool save_project(const std::string &save_path);
  bool load_project(const std::string &load_path) noexcept;

};

inline void to_json(nlohmann::json &j, const Project &p)
{
  j = nlohmann::json{
      {"name", p.name},
      {"current_frame", p.current_frame},
      {"fps", p.fps},
      {"audio_tracks", p.audio_tracks},
      {"timeline", p.timeline},
  };
}

inline void from_json(const nlohmann::json &j, Project &p)
{
  j.at("name").get_to(p.name);
  j.at("current_frame").get_to(p.current_frame);
  j.at("fps").get_to(p.fps);
  j.at("audio_tracks").get_to(p.audio_tracks);
  j.at("timeline").get_to(p.timeline);
}

#endif // NAG_ENGINE_PROJECT_H