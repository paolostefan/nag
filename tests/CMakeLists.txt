cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Fetch Google Test
# ============================================================================
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

# Windows: force gtest to use the static runtime
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ============================================================================
# Turn on testing
# ============================================================================
enable_testing()

# ============================================================================
# Tests related to the engine
# ============================================================================
add_executable(
    engine_tests
    node_serialization_test.cc
    nodes_test.cc
    schema_serialization_test.cc
)

target_link_libraries(
    engine_tests
    GTest::gtest_main
    nag_engine_lib
    nag_editor_lib
)

# ============================================================================
# Tests related to the editor
# ============================================================================
add_executable(
    editor_tests
    recent_files_manager_test.cc
    audio_playback_controller_test.cc
    timeline_controller_test.cc
    graph_scene_sync_test.cc
    command_history_test.cc
    scene_commands_test.cc
)

target_link_libraries(
    editor_tests
    GTest::gtest_main
    nag_engine_lib
    nag_editor_lib
)

# Add tests
include(GoogleTest)
gtest_discover_tests(engine_tests)
gtest_discover_tests(editor_tests)
