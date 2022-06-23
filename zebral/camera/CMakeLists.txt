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

if(MSVC)
  add_compile_options(/W4 /WX /Zc:preprocessor)
  set(EXTRA_LIBS Strmiids.lib Mfplat.lib Mf.lib)
  # add_link_options(/DELAYLOAD:MF.lib)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-c11-extensions)
  set(EXTRA_LIBS fmt::fmt)
endif()

if(OpenMP_CXX_FOUND)
  set(EXTRA_LIBS ${EXTRA_LIBS} OpenMP::OpenMP_CXX)
endif()

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
    src/param.cpp
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

# Binary and includes...
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${INC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
