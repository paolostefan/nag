#include "gtest/gtest.h"

#include <memory>

#include "editor/command_history.h"
#include "editor/scene_commands.h"

#include "editor/scene.h"
#include "engine/node_graph.h"

namespace {
  class FakeGraphCommand final : public ICommand {
  public:
    bool execute(NodeGraph &) override {
      executed_ = true;
      return true;
    }

    bool undo(NodeGraph &) override {
      undone_ = true;
      return true;
    }

    [[nodiscard]] std::string description() const override {
      return "fake graph command";
    }

    bool executed_{false};
    bool undone_{false};
  };
} // namespace

TEST(CommandHistoryTest, SceneCommandDispatchedToBoundScene) {
  Scene scene;
  NodeGraph graph;
  CommandHistory history;
  history.bind_scene(scene);

  EXPECT_TRUE(history.execute(graph, std::make_unique<AddFolderCommand>("", "Folder")));
  ASSERT_EQ(scene.root_folders.size(), 1);
  EXPECT_EQ(scene.root_folders[0]->name, "Folder");
  EXPECT_TRUE(history.can_undo());

  EXPECT_TRUE(history.undo(graph));
  EXPECT_TRUE(scene.root_folders.empty());
  EXPECT_TRUE(history.can_redo());

  EXPECT_TRUE(history.redo(graph));
  ASSERT_EQ(scene.root_folders.size(), 1);
  EXPECT_EQ(scene.root_folders[0]->name, "Folder");
}

TEST(CommandHistoryTest, SceneCommandWithoutBoundSceneFails) {
  Scene scene;
  NodeGraph graph;
  CommandHistory history;

  EXPECT_FALSE(history.execute(graph, std::make_unique<AddFolderCommand>("", "Folder")));
  EXPECT_TRUE(scene.root_folders.empty());
  EXPECT_FALSE(history.can_undo());
}

TEST(CommandHistoryTest, ExecuteSceneOverloadRejectsGraphCommands) {
  Scene scene;
  CommandHistory history;

  EXPECT_TRUE(history.execute(scene, std::make_unique<AddFolderCommand>("", "Folder")));
  ASSERT_EQ(scene.root_folders.size(), 1);

  EXPECT_FALSE(history.execute(scene, std::make_unique<FakeGraphCommand>()));
}

TEST(CommandHistoryTest, GraphCommandStillWorks) {
  Scene scene;
  NodeGraph graph;
  CommandHistory history;
  history.bind_scene(scene);

  auto command = std::make_unique<FakeGraphCommand>();
  FakeGraphCommand *raw = command.get();

  EXPECT_TRUE(history.execute(graph, std::move(command)));
  EXPECT_TRUE(raw->executed_);

  EXPECT_TRUE(history.undo(graph));
  EXPECT_TRUE(raw->undone_);

  EXPECT_TRUE(history.redo(graph));
}

TEST(CommandHistoryTest, ClearGraphCommandsKeepsSceneCommands) {
  Scene scene;
  NodeGraph graph;
  CommandHistory history;
  history.bind_scene(scene);

  history.execute(graph, std::make_unique<AddFolderCommand>("", "Folder"));
  history.execute(graph, std::make_unique<FakeGraphCommand>());
  EXPECT_EQ(history.undo_count(), 2);

  history.clear_graph_commands();

  ASSERT_EQ(history.undo_count(), 1);
  EXPECT_EQ(history.get_undo_description(), "Add folder \"Folder\"");

  EXPECT_TRUE(history.undo(graph));
  EXPECT_TRUE(scene.root_folders.empty());
}