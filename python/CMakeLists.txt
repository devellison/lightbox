cmake_minimum_required(VERSION 3.20)
project(
  zebralpy
  VERSION 0.0.2
  LANGUAGES CXX
)

find_package(pybind11 CONFIG)

# pybind11 method:
pybind11_add_module(${PROJECT_NAME} zebralpy.cpp)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE zebralcommon::zebralcommon zebralserial::zebralserial
                          zebralcam::zebralcam
)

add_subdirectory(test)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
