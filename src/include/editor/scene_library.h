#ifndef NAG_EDITOR_SCENE_LIBRARY_H
#define NAG_EDITOR_SCENE_LIBRARY_H

#include <filesystem>
#include <string>

#include "editor/scene.h"

/**
 *  SceneLibrary manages saving and loading scenes to and from disk.
 *  Scenes are stored as a single JSON file containing both scene metadata
 *  and all embedded graph data.
 */
class SceneLibrary {
public:
  SceneLibrary() = default;

  [[nodiscard]] static bool save_scene(const Scene &scene, const std::filesystem::path &path);

  [[nodiscard]] static std::unique_ptr<Scene> load_scene(const std::filesystem::path &path);
};

#endif // NAG_EDITOR_SCENE_LIBRARY_H
