cmake_minimum_required(VERSION 3.15)

project(
  lightbox_app
  VERSION ${CMAKE_PROJECT_VERSION}
  LANGUAGES CXX
)

find_package(OpenCV REQUIRED)

# Add lightbox app (move these to separate library)
set(SRC src/lightbox.cpp src/main.cpp)
set(INC inc/lightbox.hpp)
add_executable(${PROJECT_NAME} ${SRC} ${INC})
target_include_directories(${PROJECT_NAME} PUBLIC inc zebralcam::zebralcam ${OpenCV_INCLUDE_DIRS})
target_compile_definitions(
  ${PROJECT_NAME} PUBLIC LIGHTBOX_VERSION="${PROJECT_VERSION}"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} zebralcam::zebralcam ${OpenCV_LIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
