#include "gtest/gtest.h"

#include "editor/scene.h"
#include "editor/timeline_controller.h"

TEST(TimelineControllerTest, GetFrameMaxReturnsTotalFrames) {
  Scene scene("Test");
  scene.total_frames = 500;
  TimelineController controller(scene);

  EXPECT_EQ(controller.GetFrameMax(), 500);
  EXPECT_EQ(controller.GetFrameMin(), 0);
}

TEST(TimelineControllerTest, GetItemCountMatchesTimelineSize) {
  Scene scene("Test");
  TimelineController controller(scene);

  EXPECT_EQ(controller.GetItemCount(), 0);

  scene.add_timeline_segment(TimelineSegment(0, 100, "graph1"));
  scene.add_timeline_segment(TimelineSegment(0, 100, ""));

  EXPECT_EQ(controller.GetItemCount(), 2);
}

TEST(TimelineControllerTest, GetItemTypeName) {
  Scene scene("Test");
  TimelineController controller(scene);

  EXPECT_STREQ(controller.GetItemTypeName(0), "Graph");
  EXPECT_STREQ(controller.GetItemTypeName(1), "Audio");
  EXPECT_EQ(controller.GetItemTypeCount(), 2);
}

TEST(TimelineControllerTest, AddCreatesGraphSegment) {
  Scene scene("Test");
  TimelineController controller(scene);

  controller.Add(static_cast<int>(SegmentType::GRAPH));

  ASSERT_EQ(controller.GetItemCount(), 1);
  EXPECT_EQ(scene.timeline[0].type, SegmentType::GRAPH);
  EXPECT_EQ(scene.timeline[0].frame_start, 0);
  EXPECT_EQ(scene.timeline[0].frame_end, 100);
}

TEST(TimelineControllerTest, AddCreatesAudioSegment) {
  Scene scene("Test");
  TimelineController controller(scene);

  controller.Add(static_cast<int>(SegmentType::AUDIO));

  ASSERT_EQ(controller.GetItemCount(), 1);
  EXPECT_EQ(scene.timeline[0].type, SegmentType::AUDIO);
}

TEST(TimelineControllerTest, DelRemovesSegment) {
  Scene scene("Test");
  TimelineController controller(scene);

  scene.add_timeline_segment(TimelineSegment(0, 100, "g1"));
  scene.add_timeline_segment(TimelineSegment(0, 100, "g2"));
  scene.add_timeline_segment(TimelineSegment(0, 100, "g3"));
  EXPECT_EQ(controller.GetItemCount(), 3);

  controller.Del(1);

  ASSERT_EQ(controller.GetItemCount(), 2);
  EXPECT_EQ(scene.timeline[0].graph_id, "g1");
  EXPECT_EQ(scene.timeline[1].graph_id, "g3");
}

TEST(TimelineControllerTest, DelOutOfRangeIsNoop) {
  Scene scene("Test");
  TimelineController controller(scene);

  scene.add_timeline_segment(TimelineSegment(0, 100, "g1"));

  controller.Del(5);   // out of range
  controller.Del(-1);  // negative

  EXPECT_EQ(controller.GetItemCount(), 1);
}

TEST(TimelineControllerTest, DuplicateAppendsCopyAfterOriginal) {
  Scene scene("Test");
  TimelineController controller(scene);

  scene.add_timeline_segment(TimelineSegment(10, 40, "g1"));
  EXPECT_EQ(controller.GetItemCount(), 1);

  controller.Duplicate(0);

  ASSERT_EQ(controller.GetItemCount(), 2);
  EXPECT_EQ(scene.timeline[1].graph_id, "g1");
  EXPECT_EQ(scene.timeline[1].frame_start, 40);
  EXPECT_EQ(scene.timeline[1].frame_end, 70); // 40 + (40-10)
}

TEST(TimelineControllerTest, GetReturnsSegmentFields) {
  Scene scene("Test");
  TimelineController controller(scene);

  scene.add_timeline_segment(TimelineSegment(25, 150, "g1"));

  int *start = nullptr;
  int *end = nullptr;
  int type = -1;
  unsigned int color = 0;
  controller.Get(0, &start, &end, &type, &color);

  ASSERT_NE(start, nullptr);
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(*start, 25);
  EXPECT_EQ(*end, 150);
  EXPECT_EQ(type, static_cast<int>(SegmentType::GRAPH));
}

TEST(TimelineControllerTest, GetInvalidIndexDoesNothing) {
  Scene scene("Test");
  TimelineController controller(scene);

  scene.add_timeline_segment(TimelineSegment(0, 100, "g1"));

  int *start = reinterpret_cast<int *>(0x1);
  int *end = reinterpret_cast<int *>(0x2);
  controller.Get(5, &start, &end, nullptr, nullptr);

  EXPECT_EQ(start, reinterpret_cast<int *>(0x1));
  EXPECT_EQ(end, reinterpret_cast<int *>(0x2));
}

TEST(TimelineControllerTest, GetItemLabelForGraph) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *graph = scene.add_graph(*folder, "MyGraph");
  scene.add_timeline_segment(TimelineSegment(0, 100, graph->id));

  TimelineController controller(scene);
  EXPECT_STREQ(controller.GetItemLabel(0), "MyGraph");
}

TEST(TimelineControllerTest, GetItemLabelOutOfRange) {
  Scene scene("Test");
  TimelineController controller(scene);

  EXPECT_STREQ(controller.GetItemLabel(0), "");
}
