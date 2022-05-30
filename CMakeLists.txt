cmake_minimum_required(VERSION 3.15)
project(lightbox LANGUAGES CXX)
include(GoogleTest)
enable_testing()

# Automate version later
set(LIGHTBOX_VERSION "0.0.1")

# We'll want OpenCV, but probably don't care much about version
# We DO, however, want specific packages that may need to be built externally for now.
find_package(OpenCV REQUIRED opencv_core opencv_videoio opencv_highgui)

# High warning level, treat warnings as errors, compile as a console with main().
if (MSVC)
    add_compile_options(/W4 /WX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# Global defs
add_definitions(-DLIGHTBOX_VERSION="${LIGHTBOX_VERSION}")
include_directories(inc inc/camera ${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})

# Add lightbox app (move these to separate library)
set(CAMERA_SRC src/camera/camera_win.cpp
               src/camera/camera.cpp
               src/camera/camera_manager.cpp
               src/camera/camera_info.cpp
               src/camera/errors.cpp)

set(CAMERA_INC inc/camera/camera_manager.hpp
               inc/camera/camera_info.hpp
               inc/camera/camera_win.hpp
               inc/camera/camera.hpp
               inc/camera/errors.hpp)

add_library(libcamera ${CAMERA_SRC} ${CAMERA_INC})
target_link_libraries(libcamera ${OpenCV_LIBS})
target_compile_features(libcamera PUBLIC cxx_std_17)


# Add lightbox app (move these to separate library)
set(SRC src/lightbox.cpp src/main.cpp)
set(INC inc/lightbox.hpp)

add_executable(${PROJECT_NAME} ${SRC} ${INC})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries( ${PROJECT_NAME} libcamera)

# Setup GoogleTests
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Basic unit tests
add_executable(basic_tests test/basic_tests.cpp)
target_link_libraries(basic_tests gtest_main libcamera)
gtest_add_tests(TARGET basic_tests)
