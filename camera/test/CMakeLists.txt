cmake_minimum_required(VERSION 3.15)

if(MSVC)
  add_compile_options(/W4 /WX)
  set(EXTRA_LIBS Strmiids.lib Mfplat.lib Mf.lib Mfreadwrite.lib Mfuuid.lib
                 WindowsApp.lib
  )
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-c11-extensions)
  find_package(OpenCV REQUIRED)
  set(EXTRA_LIBS ${OpenCV_LIBS})
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
