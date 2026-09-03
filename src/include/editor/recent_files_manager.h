#ifndef NAG_EDITOR_RECENT_FILES_MANAGER_H
#define NAG_EDITOR_RECENT_FILES_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief Manages the list of recently opened scene files.
 *
 * Owns the in-memory list and persists it to a JSON file on disk.
 * The on-disk location is chosen by the caller at construction time.
 */
class RecentFilesManager {
public:
  explicit RecentFilesManager(std::filesystem::path path);

  /// @brief Load the recent-files list from disk (path given at construction).
  void load();

  /// @brief Save the current list to disk (path given at construction).
  void save() const;

  /// @brief Add a path to the front of the list, deduplicating and capping.
  void add(const std::string &path);

  /// @brief Remove all occurrences of a path from the list.
  void remove(const std::string &path);

  [[nodiscard]] const std::vector<std::string> &get() const { return recent_; }

  [[nodiscard]] std::filesystem::path path() const { return path_; }

private:
  std::filesystem::path path_;
  std::vector<std::string> recent_;
  static constexpr size_t kMaxItems{10};
};

#endif // NAG_EDITOR_RECENT_FILES_MANAGER_H
