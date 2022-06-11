cmake_minimum_required(VERSION 3.20)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

find_package(OpenMP)

if(MSVC)
  add_compile_options(/W4 /WX /Zc:preprocessor)
  set(EXTRA_LIBS Strmiids.lib Mfplat.lib Mf.lib delayimp.lib)
  # add_link_options(/DELAYLOAD:Mf.lib)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-c11-extensions)
  find_package(OpenCV REQUIRED highgui core)
  find_package(fmt REQUIRED)
  set(EXTRA_LIBS ${OpenCV_LIBS} fmt::fmt)
endif()

if(OpenMP_CXX_FOUND)
  set(EXTRA_LIBS ${EXTRA_LIBS} OpenMP::OpenMP_CXX)
endif()

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
target_link_libraries(camera_tests gtest_main zebralcam ${EXTRA_LIBS})
gtest_add_tests(TARGET camera_tests)
