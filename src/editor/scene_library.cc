#include "editor/scene_library.h"

#include <fstream>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using json = nlohmann::json;

SceneLibrary::SceneLibrary(std::filesystem::path scenes_directory)
  : scenes_directory_(std::move(scenes_directory)) {
  if (!std::filesystem::exists(scenes_directory_)) {
    std::filesystem::create_directories(scenes_directory_);
    spdlog::info("Created scenes directory: {}", scenes_directory_.string());
  }
}

bool SceneLibrary::save_scene(const Scene &scene, const std::filesystem::path &path) {
  try {
    json j = scene;
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
    scene->pristine = true;
    spdlog::info("Loaded scene from: {}", path.string());
    return scene;
  } catch (const std::exception &e) {
    spdlog::error("Failed to load scene: {}", e.what());
    return nullptr;
  }
}

bool SceneLibrary::save_graph(const NodeGraph &graph, const GraphReference &reference) const {
  const auto full_path = scenes_directory_ / reference.path;

  if (const auto parent_dir = full_path.parent_path(); !std::filesystem::exists(parent_dir)) {
    std::filesystem::create_directories(parent_dir);
  }

  auto result = serializer_.save(graph, full_path.string());
  if (!result.success) {
    spdlog::error("Failed to save graph: {}", result.error_message);
    return false;
  }

  spdlog::info("Saved graph to: {}", full_path.string());
  return true;
}

std::unique_ptr<NodeGraph> SceneLibrary::load_graph(const GraphReference &reference) {
  return load_graph(reference.id);
}

std::unique_ptr<NodeGraph> SceneLibrary::load_graph(const std::string & /* graph_id */) {
  return nullptr;
}

bool SceneLibrary::delete_graph(const GraphReference &reference) const {
  const auto full_path = scenes_directory_ / reference.path;
  if (std::filesystem::exists(full_path)) {
    std::filesystem::remove(full_path);
    spdlog::info("Deleted graph: {}", full_path.string());
    return true;
  }
  spdlog::warn("Graph file not found for deletion: {}", full_path.string());
  return false;
}

bool SceneLibrary::rename_graph(const GraphReference &reference, const std::string &new_name) {
  GraphReference modified = reference;
  modified.name = new_name;
  return true;
}
