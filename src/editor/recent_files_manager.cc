#include "editor/recent_files_manager.h"

#include <algorithm>
#include <fstream>

#include "nlohmann/json.hpp"

RecentFilesManager::RecentFilesManager(std::filesystem::path path)
  : path_(std::move(path)) {
}

void RecentFilesManager::load() {
  std::ifstream file(path_);
  if (!file.is_open()) return;
  try {
    nlohmann::json j;
    file >> j;
    recent_ = j.get<std::vector<std::string> >();
    // Prune non-existent files
    std::erase_if(recent_, [](const std::string &p) { return !std::filesystem::exists(p); });
  } catch (...) {
    recent_.clear();
  }
}

void RecentFilesManager::save() const {
  const nlohmann::json j = recent_;
  std::ofstream file(path_);
  if (file.is_open()) {
    file << j.dump(2);
  }
}

void RecentFilesManager::add(const std::string &path) {
  // Move to front (remove duplicate if exists)
  auto it = std::find(recent_.begin(), recent_.end(), path);
  if (it != recent_.end()) {
    recent_.erase(it);
  }
  recent_.insert(recent_.begin(), path);
  // Trim to max
  if (recent_.size() > kMaxItems) {
    recent_.resize(kMaxItems);
  }
}

void RecentFilesManager::remove(const std::string &path) {
  std::erase(recent_, path);
}
