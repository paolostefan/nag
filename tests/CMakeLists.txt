cmake_minimum_required(VERSION 3.14)

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
# Timeline Test
# ============================================================================
add_executable(
  timeline_test
        timeline_json_test.cc
        timeline_test.cc
)

# Link Google Test and nlohmann/json to the test executable
target_link_libraries(
  timeline_test
  GTest::gtest_main
  nag_editor_lib
)

# Include directories for the test executable
target_include_directories(
  timeline_test PRIVATE
  ${CMAKE_SOURCE_DIR}/src/include
)

# Add tests
include(GoogleTest)
gtest_discover_tests(timeline_test)
