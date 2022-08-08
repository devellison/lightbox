# Lightbox project
#
# Custom hardware + software for an experimental computer vision lightbox.
#
cmake_minimum_required(VERSION 3.20)

project(
  lightbox
  VERSION 0.0.2
  LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(MSVC)

else()
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3")
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)

# Package info
set(CPACK_PACKAGE_VENDOR "zebral.net")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Zebral Lightbox Project")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
include(CPack)

include(GNUInstallDirs)
include(CMakePrintHelpers)
include(GoogleTest)
enable_testing()

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED highgui core)
find_package(Doxygen REQUIRED dot OPTIONAL_COMPONENTS mscgen dia)
find_package(OpenMP)

# Setup gtest Note - this seems kinda slow/repeatedly run. Moving to global for
# now.
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE
)

# FetchContent_MakeAvailable(googletest) Hack to prevent installing all the
# googletest stuff with ours
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(
    ${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL
  )
endif()

#set(NANOGUI_BUILD_PYTHON OFF CACHE INTERNAL "")
#set(NANOGUI_BUILD_SHARED OFF CACHE INTERNAL "")
#FetchContent_Declare(
#  NanoGUI
#  GIT_REPOSITORY https://github.com/wjakob/nanogui.git
#)
#FetchContent_MakeAvailable(NanoGUI)
# Disable building extras we won't need (pure C++ project)
set(NANOGUI_BUILD_EXAMPLE OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
set(NANOGUI_INSTALL       OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_SHARED  OFF CACHE BOOL " " FORCE)

# Add the configurations from nanogui
add_subdirectory(ext/nanogui)

# For reliability of parallel build, make the NanoGUI targets dependencies
set_property(TARGET glfw glfw_objects nanogui PROPERTY FOLDER "dependencies")


# High warning level, treat warnings as errors, compile as a console with
# main().
if(MSVC)
  add_compile_options(/W4 /WX)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup"
  )
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
  find_package(fmt REQUIRED)
endif()

add_subdirectory(zebral)
add_subdirectory(app)
add_subdirectory(test)

set(DOXYGEN_FILE_PATTERNS
    *.hpp
    *.cpp
    *.py
    *.md
    *.h
    *.c
    *.asm
    *.js
    *.java
)
set(DOXYGEN_PREDEFINED _WIN32 __linux__)
set(DOXYGEN_GRAPHICAL_HIERARCHY YES)
set(DOXYGEN_DIRECTORY_GRAPH YES)
set(DOXYGEN_DOT_IMAGE_FORMAT svg)
set(DOXYGEN_INTERACTIVE_SVG YES)
set(DOXYGEN_CALL_GRAPH YES)
set(DOXYGEN_CALLER_GRAPH YES)
set(DOXYGEN_WARN_NO_PARAMDOC YES)
set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
set(DOXYGEN_MULTILINE_CPP_IS_BRIEF YES)
set(DOXYGEN_CLASS_GRAPH YES)
set(DOXYGEN_COLLABORATION_GRAPH YES)
set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_docs)
set(DOXYGEN_EXCLUDE */build/* */_deps/*)
set(DOXYGEN_WARN_LOGFILE ${CMAKE_SOURCE_DIR}/undocumented.txt)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
doxygen_add_docs(
  ${PROJECT_NAME}_docs
  ${CMAKE_CURRENT_SOURCE_DIR}/README.md
  ${PROJECT_NAME}_docs
  ${CMAKE_CURRENT_SOURCE_DIR}/zebral/camera/README.md
  ${CMAKE_CURRENT_SOURCE_DIR}/zebral/serial/README.md
  ${CMAKE_CURRENT_SOURCE_DIR}/app/src
  ${CMAKE_CURRENT_SOURCE_DIR}/app/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/zebral/camera/src
  ${CMAKE_CURRENT_SOURCE_DIR}/zebral/camera/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/zebral/serial/src
  ${CMAKE_CURRENT_SOURCE_DIR}/zebral/serial/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/
  # ALL
  COMMENT "Generating lightbox docs"
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_docs
        DESTINATION ${CMAKE_INSTALL_INFODIR}
)
