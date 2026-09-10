#include "gtest/gtest.h"

#include "editor/scene_commands.h"

#include "editor/scene.h"

TEST(SceneCommandsTest, AddFolderExecuteUndoRedo) {
  Scene scene;
  AddFolderCommand cmd("", "Folder");

  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(scene.root_folders.size(), 1);
  const std::string id = scene.root_folders[0]->id;
  EXPECT_EQ(scene.root_folders[0]->name, "Folder");

  EXPECT_TRUE(cmd.undo(scene));
  EXPECT_TRUE(scene.root_folders.empty());

  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(scene.root_folders.size(), 1);
  EXPECT_EQ(scene.root_folders[0]->id, id);
  EXPECT_EQ(scene.root_folders[0]->name, "Folder");

  EXPECT_TRUE(cmd.undo(scene));
  EXPECT_TRUE(scene.root_folders.empty());
}

TEST(SceneCommandsTest, AddFolderUnderParent) {
  Scene scene;
  auto *parent = scene.add_folder(nullptr, "Parent");
  AddFolderCommand cmd(parent->id, "Child");

  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(parent->children.size(), 1);
  EXPECT_EQ(parent->children[0]->name, "Child");
  const std::string id = parent->children[0]->id;

  EXPECT_TRUE(cmd.undo(scene));
  EXPECT_TRUE(parent->children.empty());

  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(parent->children.size(), 1);
  EXPECT_EQ(parent->children[0]->id, id);
}

TEST(SceneCommandsTest, RemoveFolderCapturesSubtree) {
  Scene scene;
  auto *parent = scene.add_folder(nullptr, "Parent");
  auto *child = scene.add_folder(parent->id, "Child");
  [[maybe_unused]] auto *graph = scene.add_graph(*child, "G");

  RemoveFolderCommand cmd(parent->id);
  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_TRUE(scene.root_folders.empty());

  EXPECT_TRUE(cmd.undo(scene));
  ASSERT_EQ(scene.root_folders.size(), 1);
  EXPECT_EQ(scene.root_folders[0]->name, "Parent");
  ASSERT_EQ(scene.root_folders[0]->children.size(), 1);
  EXPECT_EQ(scene.root_folders[0]->children[0]->name, "Child");
  ASSERT_EQ(scene.root_folders[0]->children[0]->graphs.size(), 1);
  EXPECT_EQ(scene.root_folders[0]->children[0]->graphs[0].name, "G");

  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_TRUE(scene.root_folders.empty());
}

TEST(SceneCommandsTest, RenameFolderExecuteUndo) {
  Scene scene;
  auto *folder = scene.add_folder(nullptr, "A");
  RenameFolderCommand cmd(folder->id, "B");

  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_EQ(scene.find_folder(folder->id)->name, "B");

  EXPECT_TRUE(cmd.undo(scene));
  EXPECT_EQ(scene.find_folder(folder->id)->name, "A");

  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_EQ(scene.find_folder(folder->id)->name, "B");
}

TEST(SceneCommandsTest, AddGraphPredeterminedId) {
  Scene scene;
  auto *folder = scene.add_folder(nullptr, "Root");
  AddGraphCommand cmd(folder->id, "NewGraph");
  const std::string id = cmd.graph_id();
  EXPECT_FALSE(id.empty());

  EXPECT_TRUE(cmd.execute(scene));
  const GraphReference *ref = scene.find_graph(id);
  ASSERT_NE(ref, nullptr);
  EXPECT_EQ(ref->name, "NewGraph");
  EXPECT_TRUE(scene.graph_data.contains(id));

  EXPECT_TRUE(cmd.undo(scene));
  EXPECT_TRUE(scene.find_graph(id) == nullptr);
  EXPECT_FALSE(scene.graph_data.contains(id));

  EXPECT_TRUE(cmd.execute(scene));
  ref = scene.find_graph(id);
  ASSERT_NE(ref, nullptr);
  EXPECT_EQ(ref->name, "NewGraph");
}

TEST(SceneCommandsTest, RemoveGraphRestoresReferenceAndData) {
  Scene scene;
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *graph = scene.add_graph(*folder, "G");
  scene.graph_data[graph->id] = {{"foo", 42}};

  RemoveGraphCommand cmd(graph->id);
  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_TRUE(scene.find_graph(graph->id) == nullptr);
  EXPECT_FALSE(scene.graph_data.contains(graph->id));

  EXPECT_TRUE(cmd.undo(scene));
  const GraphReference *restored = scene.find_graph(graph->id);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->name, "G");
  ASSERT_TRUE(scene.graph_data.contains(graph->id));
  EXPECT_EQ(scene.graph_data.at(graph->id).at("foo"), 42);

  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_TRUE(scene.find_graph(graph->id) == nullptr);
  EXPECT_FALSE(scene.graph_data.contains(graph->id));
}

// Regression: the graph's containing folder index (in its parent vector) must
// not be conflated with the graph's index inside that folder. Removing and
// undoing must restore the exact graph, not whatever lives at a mirrored index.
TEST(SceneCommandsTest, RemoveGraphRestoresCorrectGraphWhenIndexesDiffer) {
  Scene scene;
  auto *folder_a = scene.add_folder(nullptr, "A");
  auto *graph_a = scene.add_graph(*folder_a, "G_A");
  auto *folder_b = scene.add_folder(nullptr, "B");
  auto *graph_b = scene.add_graph(*folder_b, "G_B");
  scene.graph_data[graph_b->id] = {{"bar", 7}};

  RemoveGraphCommand cmd(graph_b->id);
  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_TRUE(scene.find_graph(graph_b->id) == nullptr);
  EXPECT_TRUE(scene.find_graph(graph_a->id) != nullptr);

  EXPECT_TRUE(cmd.undo(scene));
  const GraphReference *restored = scene.find_graph(graph_b->id);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->name, "G_B");
  EXPECT_EQ(&restored->dirty, &folder_b->graphs.front().dirty);
  ASSERT_TRUE(scene.graph_data.contains(graph_b->id));
  EXPECT_EQ(scene.graph_data.at(graph_b->id).at("bar"), 7);
}

TEST(SceneCommandsTest, RenameGraphExecuteUndo) {
  Scene scene;
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *graph = scene.add_graph(*folder, "A");
  RenameGraphCommand cmd(graph->id, "B");

  EXPECT_TRUE(cmd.execute(scene));
  EXPECT_EQ(scene.find_graph(graph->id)->name, "B");

  EXPECT_TRUE(cmd.undo(scene));
  EXPECT_EQ(scene.find_graph(graph->id)->name, "A");
}

TEST(SceneCommandsTest, AddTimelineSegmentExecuteUndoRedo) {
  Scene scene;
  AddTimelineSegmentCommand cmd(TimelineSegment(0, 100, "g1"));

  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(scene.timeline.size(), 1);
  EXPECT_EQ(scene.timeline[0].graph_id, "g1");

  EXPECT_TRUE(cmd.undo(scene));
  EXPECT_TRUE(scene.timeline.empty());

  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(scene.timeline.size(), 1);
  EXPECT_EQ(scene.timeline[0].graph_id, "g1");
}

TEST(SceneCommandsTest, RemoveTimelineSegmentReinsertsAtIndex) {
  Scene scene;
  scene.add_timeline_segment(TimelineSegment(0, 100, "a"));
  scene.add_timeline_segment(TimelineSegment(0, 100, "b"));
  scene.add_timeline_segment(TimelineSegment(0, 100, "c"));

  RemoveTimelineSegmentCommand cmd(1);
  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(scene.timeline.size(), 2);
  EXPECT_EQ(scene.timeline[0].graph_id, "a");
  EXPECT_EQ(scene.timeline[1].graph_id, "c");

  EXPECT_TRUE(cmd.undo(scene));
  ASSERT_EQ(scene.timeline.size(), 3);
  EXPECT_EQ(scene.timeline[0].graph_id, "a");
  EXPECT_EQ(scene.timeline[1].graph_id, "b");
  EXPECT_EQ(scene.timeline[2].graph_id, "c");

  EXPECT_TRUE(cmd.execute(scene));
  ASSERT_EQ(scene.timeline.size(), 2);
  EXPECT_EQ(scene.timeline[1].graph_id, "c");
}