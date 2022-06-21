cmake_minimum_required(VERSION 3.20)

project(
  zebral_camera_util
  VERSION ${CMAKE_PROJECT_VERSION}
  LANGUAGES CXX
)

# Add lightbox app (move these to separate library)
set(ZEB_SRC zebral_camera_util.cpp)
set(ZEB_INC)

add_executable(${PROJECT_NAME} ${ZEB_SRC} ${ZEB_INC})
target_include_directories(
  ${PROJECT_NAME} PUBLIC zebralcam::zebralcam ${OpenCV_INCLUDE_DIRS}
)
target_compile_definitions(
  ${PROJECT_NAME} PUBLIC LIGHTBOX_VERSION="${PROJECT_VERSION}"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(${PROJECT_NAME} zebralcam::zebralcam ${OpenCV_LIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
