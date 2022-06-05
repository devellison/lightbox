# Lightbox project
#
# Custom hardware + software for an experimental computer vision lightbox.
#
cmake_minimum_required(VERSION 3.15)

project(
  lightbox
  VERSION 0.0.2
  LANGUAGES CXX
)

# Package info
set(CPACK_PACKAGE_VENDOR "zebral.net")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ZeBrAl Lightbox Project")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES /.git /.*build.* /\\\\.DS_Store)
include(CPack)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)
include(CMakePrintHelpers)
include(GoogleTest)
enable_testing()

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Doxygen REQUIRED dot OPTIONAL_COMPONENTS mscgen dia)

# High warning level, treat warnings as errors, compile as a console with
# main().
if(MSVC)
  add_compile_options(/W4 /WX)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup"
  )
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_subdirectory(camera)
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
doxygen_add_docs(
  ${PROJECT_NAME}_docs ${CMAKE_CURRENT_SOURCE_DIR}/README.md
  ${CMAKE_CURRENT_SOURCE_DIR}/app/
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/ ALL COMMENT "Generating lightbox docs"
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_docs DESTINATION ${CMAKE_INSTALL_INFODIR})
