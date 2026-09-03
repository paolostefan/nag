#include <filesystem>
#include <fstream>

#include "gtest/gtest.h"

#include "editor/recent_files_manager.h"

namespace fs = std::filesystem;

namespace {

  fs::path make_temp_dir() {
    const fs::path dir = fs::temp_directory_path() /
      ("nag_recent_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()));
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
  }

  void touch(const fs::path &p) {
    std::ofstream(p).close();
  }

} // namespace

TEST(RecentFilesManagerTest, AddStoresInOrder) {
  RecentFilesManager mgr(fs::temp_directory_path() / "nag_recent_test.json");
  mgr.add("/a");
  mgr.add("/b");
  mgr.add("/c");

  ASSERT_EQ(mgr.get().size(), 3u);
  EXPECT_EQ(mgr.get()[0], "/c");
  EXPECT_EQ(mgr.get()[1], "/b");
  EXPECT_EQ(mgr.get()[2], "/a");
}

TEST(RecentFilesManagerTest, AddDeduplicates) {
  RecentFilesManager mgr(fs::temp_directory_path() / "nag_recent_test.json");
  mgr.add("/a");
  mgr.add("/b");
  mgr.add("/a");

  ASSERT_EQ(mgr.get().size(), 2u);
  EXPECT_EQ(mgr.get()[0], "/a");
  EXPECT_EQ(mgr.get()[1], "/b");
}

TEST(RecentFilesManagerTest, AddCapsAtMax) {
  RecentFilesManager mgr(fs::temp_directory_path() / "nag_recent_test.json");
  for (int i = 0; i < 15; ++i) {
    mgr.add("/path_" + std::to_string(i));
  }

  ASSERT_EQ(mgr.get().size(), 10u);
  EXPECT_EQ(mgr.get().front(), "/path_14");
  EXPECT_EQ(mgr.get().back(), "/path_5");
}

TEST(RecentFilesManagerTest, RemoveErasesAllOccurrences) {
  RecentFilesManager mgr(fs::temp_directory_path() / "nag_recent_test.json");
  mgr.add("/a");
  mgr.add("/b");
  mgr.add("/a");

  mgr.remove("/a");

  ASSERT_EQ(mgr.get().size(), 1u);
  EXPECT_EQ(mgr.get()[0], "/b");
}

TEST(RecentFilesManagerTest, SaveThenLoadRoundTrip) {
  const fs::path dir = make_temp_dir();
  const fs::path file = dir / "recent.json";
  const fs::path existing = dir / "existing.nagscene";
  touch(existing);

  {
    RecentFilesManager mgr(file);
    mgr.add(existing.string());
    mgr.add("/does_not_exist.nagscene");
    mgr.save();
  }

  RecentFilesManager loaded(file);
  loaded.load();

  // The non-existent path is pruned on load
  ASSERT_EQ(loaded.get().size(), 1u);
  EXPECT_EQ(loaded.get()[0], existing.string());
}

TEST(RecentFilesManagerTest, LoadMissingFileIsEmpty) {
  const fs::path dir = make_temp_dir();
  RecentFilesManager mgr(dir / "does_not_exist.json");
  mgr.load();

  EXPECT_TRUE(mgr.get().empty());
}
