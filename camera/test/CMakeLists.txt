cmake_minimum_required(VERSION 3.15)

# Setup gtest Note - this seems kinda slow/repeatedly run from VSCode.  Fix.
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE
)
FetchContent_MakeAvailable(googletest)

# Basic unit tests
add_executable(camera_tests camera_tests.cpp)
target_link_libraries(camera_tests gtest_main zebralcam)
gtest_add_tests(TARGET camera_tests)
