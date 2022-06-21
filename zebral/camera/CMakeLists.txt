# ZebralCam library
#
# Provides cross-platform cameras with control options and hardware
# identification
cmake_minimum_required(VERSION 3.20)

project(
  zebralcam
  VERSION 0.0.2
  LANGUAGES CXX
)

# If we're being run directly, package the camera library. Otherwise let it be
# part of the parent package.
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(CPACK_PACKAGE_VENDOR "zebral.net")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ZebralCam Library")
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
  set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
  include(CPack)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

# find_package(OpenMP)

if(MSVC)
  add_compile_options(/W4 /WX /Zc:preprocessor)
  set(EXTRA_LIBS Strmiids.lib Mfplat.lib Mf.lib)
  # add_link_options(/DELAYLOAD:MF.lib)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-c11-extensions)
  find_package(fmt REQUIRED)
  set(EXTRA_LIBS fmt::fmt)
endif()

if(OpenMP_CXX_FOUND)
  set(EXTRA_LIBS ${EXTRA_LIBS} OpenMP::OpenMP_CXX)
endif()

find_package(Threads REQUIRED)
find_package(Doxygen REQUIRED dot OPTIONAL_COMPONENTS mscgen dia)
include(GoogleTest)
enable_testing()

# Add camera library

set(SRC
    src/device_v4l2.cpp
    src/camera.cpp
    src/camera_winrt.cpp
    src/camera_v4l2.cpp
    src/camera_manager.cpp
    src/camera_info.cpp
    src/buffer_memmap.cpp
    src/convert.cpp
)

set(INC
    inc/device_v4l2.hpp
    inc/buffer_memmap.hpp
    inc/camera_manager.hpp
    inc/camera_info.hpp
    inc/camera_platform.hpp
    inc/camera.hpp
    inc/param.hpp
    inc/convert.hpp
)

add_library(${PROJECT_NAME} ${SRC} ${INC})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(
  ${PROJECT_NAME} PUBLIC inc ${fmt_INCLUDE_DIRS} zebralcommon::zebralcommon
)
target_link_libraries(
  ${PROJECT_NAME} PUBLIC ${EXTRA_LIBS} Threads::Threads
                         zebralcommon::zebralcommon
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

add_subdirectory(test)
add_subdirectory(utils)
# Only do our own doxygen stuff if we're being run directly.
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
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
  set(DOXYGEN_EXCLUDE */build/* */_deps/*)
  set(DOXYGEN_WARN_LOGFILE ${CMAKE_SOURCE_DIR}/undocumented.txt)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_docs)
  doxygen_add_docs(
    ${PROJECT_NAME}_docs README.md ${SRC} ${INC} USE_STAMP_FILE
    # ALL
    COMMENT "Generating camera docs"
  )
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_docs
          DESTINATION ${CMAKE_INSTALL_INFODIR}
  )
endif()

# Binary and includes...
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${INC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export a cmake file for package finding
export(
  TARGETS ${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
  FILE ${PROJECT_NAME}Targets.cmake
)
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(PACKAGE ${PROJECT_NAME})
