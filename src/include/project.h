#ifndef NAG_ENGINE_PROJECT_H
#define NAG_ENGINE_PROJECT_H

#include <filesystem>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

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

  inline void add_audio_track(const std::string &path,
                              double start_seconds = 0.0)
  {
    AudioTrack track;
    track.name = std::filesystem::path(path).stem().string();
    track.path = path;
    track.start_seconds = start_seconds;

    audio_tracks.push_back(track);
    pristine = false;
  }

  inline bool save(const std::string &save_path)
  {
    if (pristine)
    {
      spdlog::error("Cowardly refusing to save an unmodified project");
      return false;
    }

    spdlog::info("Saving project to '{}'...", save_path);
    path = save_path;

    bool result = false;

    try
    {
      nlohmann::json j;
      j["name"] = name;
      j["audio_tracks"] = nlohmann::json::array();

      for (const auto &track : audio_tracks)
      {
        nlohmann::json track_json;
        track_json["name"] = track.name;
        track_json["path"] = track.path;
        track_json["start_seconds"] = track.start_seconds;

        j["audio_tracks"].push_back(track_json);
      }

      {
        std::ofstream save_file(save_path);
        save_file << std::setw(2) << j << std::endl;
      }

      pristine = true;
      result = true;

      spdlog::info("Project saved successfully");
    }
    catch (const std::exception &e)
    {
      spdlog::error("Failed to save project: {}", e.what());
    }

    return result;
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