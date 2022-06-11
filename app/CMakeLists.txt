cmake_minimum_required(VERSION 3.20)

project(
  lightbox_app
  VERSION ${CMAKE_PROJECT_VERSION}
  LANGUAGES CXX
)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenCV REQUIRED highgui core)
find_package(OpenMP)

if(MSVC)
  add_compile_options(/W4 /WX /Zc:preprocessor)
  set(EXTRA_LIBS Strmiids.lib Mfplat.lib Mf.lib delayimp.lib)
  # add_link_options(/DELAYLOAD:Mf.lib)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-c11-extensions)
endif()

if(OpenMP_CXX_FOUND)
  set(EXTRA_LIBS ${EXTRA_LIBS} OpenMP::OpenMP_CXX)
endif()

# Add lightbox app (move these to separate library)
set(SRC src/lightbox.cpp src/main.cpp)
set(INC inc/lightbox.hpp)
add_executable(${PROJECT_NAME} ${SRC} ${INC})
target_include_directories(
  ${PROJECT_NAME} PUBLIC inc zebralcam::zebralcam ${OpenCV_INCLUDE_DIRS}
)
target_compile_definitions(
  ${PROJECT_NAME} PUBLIC LIGHTBOX_VERSION="${PROJECT_VERSION}"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(${PROJECT_NAME} zebralcam::zebralcam ${OpenCV_LIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
