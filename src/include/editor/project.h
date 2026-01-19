#ifndef NAG_ENGINE_PROJECT_H
#define NAG_ENGINE_PROJECT_H

#include <filesystem>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "engine/audio_track.h"

class Project
{
public:
  Project() = default;

  Project(const std::string &name) : name(name) {}

  const std::string &get_name() const noexcept { return name; }
  const std::filesystem::path &get_path() const noexcept { return path; }

  const std::vector<AudioTrack> &get_audio_tracks() const noexcept
  {
    return audio_tracks;
  }

  inline void add_audio_track(const std::string &path,
                              const double start_seconds = 0.0)
  {
    audio_tracks.emplace_back(path, start_seconds);
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
        track_json["title"] = track.get_title();
        track_json["path"] = track.get_path();
        track_json["start_seconds"] = track.get_start_seconds();

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

  inline bool load(const std::string &load_path) noexcept
  {
    path = load_path;

    bool result = false; // Assume failure
    try
    {
      std::ifstream load_file(load_path);
      nlohmann::json j;
      load_file >> j;

      name = j["name"];
      audio_tracks.clear();

      for (const auto &track : j["audio_tracks"])
      {
        audio_tracks.emplace_back(
            track["path"].get<std::filesystem::path>(),
            track["start_seconds"].get<double>());
      }

      result = true;
    }
    catch (const std::exception &e)
    {
      spdlog::error("Failed to load project: {}", e.what());
    }

    return result;
  }

public:
  int current_frame{0};
  uint8_t fps{60};

private:
  /// Path to the project descriptor file
  std::filesystem::path path{};

  /// Project name
  std::string name{"Void of space"};

  std::vector<AudioTrack> audio_tracks{};

  bool pristine{true};
};

#endif // NAG_ENGINE_PROJECT_H