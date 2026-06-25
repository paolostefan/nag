#include "editor/scene_library.h"

#include <fstream>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using json = nlohmann::json;

bool SceneLibrary::save_scene(const Scene &scene, const std::filesystem::path &path) {
  try {
    const json j = scene;
    std::ofstream file(path);
    if (!file.is_open()) {
      spdlog::error("Failed to open scene file for writing: {}", path.string());
      return false;
    }
    file << j.dump(2);
    spdlog::info("Saved scene to: {}", path.string());
    return true;
  } catch (const std::exception &e) {
    spdlog::error("Failed to save scene: {}", e.what());
    return false;
  }
}

std::unique_ptr<Scene> SceneLibrary::load_scene(const std::filesystem::path &path) {
  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      spdlog::error("Failed to open scene file for reading: {}", path.string());
      return nullptr;
    }
    json j;
    file >> j;
    auto scene = std::make_unique<Scene>();
    j.get_to(*scene);
    scene->path = path;
    scene->dirty = false;
    spdlog::info("Loaded scene from: {}", path.string());
    return scene;
  } catch (const std::exception &e) {
    spdlog::error("Failed to load scene: {}", e.what());
    return nullptr;
  }
}
