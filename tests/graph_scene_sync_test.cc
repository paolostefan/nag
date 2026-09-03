#include <gtest/gtest.h>
#include <SDL.h>
#include <GL/glew.h>

#include "editor/graph_scene_sync.h"
#include "editor/scene.h"
#include "engine/node_registry.h"
#include "engine/nodes/temporal_nodes.h"
#include "engine/nodes/generator_nodes.h"

// ============================================================================
// Fixture – SDL/GL context needed because graph deserialization creates nodes
// (e.g. TextureLoaderNode) whose constructors call OpenGL.
// ============================================================================
class GraphSceneSyncTest : public testing::Test {
protected:
  SDL_Window *window_{nullptr};
  SDL_GLContext gl_context_{nullptr};

  void SetUp() override {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    window_ = SDL_CreateWindow("Test", 0, 0, 1, 1,
                               SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    gl_context_ = SDL_GL_CreateContext(window_);
    glewInit();

    register_all_builtin_nodes();
  }

  void TearDown() override {
    if (gl_context_) SDL_GL_DeleteContext(gl_context_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
  }
};

static GraphReference *make_ref(Scene &scene, GraphFolder &folder,
                                const char *name) {
  return scene.add_graph(folder, name);
}

TEST_F(GraphSceneSyncTest, FindFirstGraphReturnsNullWhenEmpty) {
  Scene scene("Test");
  GraphSceneSync sync(scene);

  EXPECT_EQ(sync.find_first_graph(), nullptr);
}

TEST_F(GraphSceneSyncTest, FindFirstGraphFindsRootGraph) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *g1 = make_ref(scene, *folder, "A");
  GraphSceneSync sync(scene);

  EXPECT_EQ(sync.find_first_graph(), g1);
}

TEST_F(GraphSceneSyncTest, FindFirstGraphSearchesNestedFolders) {
  Scene scene("Test");
  auto *root = scene.add_folder(nullptr, "Root");
  auto *sub = scene.add_folder(root->id, "Sub");
  auto *deep = scene.add_folder(sub->id, "Deep");
  auto *g = make_ref(scene, *deep, "Nested");
  GraphSceneSync sync(scene);

  EXPECT_EQ(sync.find_first_graph(), g);
}

TEST_F(GraphSceneSyncTest, FindFirstGraphPrefersShallowerGraph) {
  Scene scene("Test");
  auto *root = scene.add_folder(nullptr, "Root");
  auto *g_top = make_ref(scene, *root, "Top");
  auto *sub = scene.add_folder(root->id, "Sub");
  make_ref(scene, *sub, "Nested");
  GraphSceneSync sync(scene);

  EXPECT_EQ(sync.find_first_graph(), g_top);
}

TEST_F(GraphSceneSyncTest, SyncGraphSerializesAndMarksClean) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *ref = make_ref(scene, *folder, "G");
  GraphSceneSync sync(scene);

  NodeGraph graph;
  auto time = TimeNode::create();
  time->name = "Time";
  graph.add_node(std::move(time));

  ref->dirty = true;
  sync.sync_graph(*ref, graph);

  EXPECT_FALSE(ref->dirty);
  const auto it = scene.graph_data.find(ref->id);
  ASSERT_NE(it, scene.graph_data.end());
  EXPECT_FALSE(it->second.is_null());
}

TEST_F(GraphSceneSyncTest, LoadGraphRoundTripsNodes) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *ref = make_ref(scene, *folder, "G");
  GraphSceneSync sync(scene);

  NodeGraph graph;
  auto time = TimeNode::create();
  time->name = "MyTime";
  graph.add_node(std::move(time));

  sync.sync_graph(*ref, graph);

  auto loaded = sync.load_graph(*ref);
  ASSERT_TRUE(loaded.has_value());
  ASSERT_EQ(loaded->nodes.size(), 1);
  EXPECT_EQ(loaded->nodes[0]->name, "MyTime");
}

TEST_F(GraphSceneSyncTest, LoadGraphReturnsNulloptWhenGGraphDataMissing) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *ref = make_ref(scene, *folder, "G");
  GraphSceneSync sync(scene);

  EXPECT_FALSE(sync.load_graph(*ref).has_value());
}

TEST_F(GraphSceneSyncTest, LoadGraphReturnsNulloptForNullData) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *ref = make_ref(scene, *folder, "G");
  scene.graph_data[ref->id] = nullptr;
  GraphSceneSync sync(scene);

  EXPECT_FALSE(sync.load_graph(*ref).has_value());
}

TEST_F(GraphSceneSyncTest, AddGraphCreatesReferenceWithEmptyData) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  GraphSceneSync sync(scene);

  auto *ref = sync.add_graph(*folder, "Brand New");

  ASSERT_NE(ref, nullptr);
  EXPECT_EQ(&folder->graphs.back(), ref);
  EXPECT_EQ(ref->name, "Brand New");
  ASSERT_NE(scene.graph_data.find(ref->id), scene.graph_data.end());
  EXPECT_TRUE(scene.graph_data[ref->id].is_null());
}

TEST_F(GraphSceneSyncTest, AddGraphAppendsToGivenFolder) {
  Scene scene("Test");
  GraphFolder folder("f-id", "Folder");
  GraphSceneSync sync(scene);

  auto *ref = sync.add_graph(folder, "X");

  ASSERT_NE(ref, nullptr);
  ASSERT_EQ(folder.graphs.size(), 1);
  EXPECT_EQ(&folder.graphs.front(), ref);
}

TEST_F(GraphSceneSyncTest, RemoveGraphDataErasesEntry) {
  Scene scene("Test");
  auto *folder = scene.add_folder(nullptr, "Root");
  auto *ref = make_ref(scene, *folder, "G");
  scene.graph_data[ref->id] = nullptr;
  GraphSceneSync sync(scene);

  sync.remove_graph_data(ref->id);

  EXPECT_EQ(scene.graph_data.find(ref->id), scene.graph_data.end());
}

TEST_F(GraphSceneSyncTest, RemoveGraphDataMissingIsNoop) {
  Scene scene("Test");
  GraphSceneSync sync(scene);

  EXPECT_NO_THROW(sync.remove_graph_data("does-not-exist"));
}
