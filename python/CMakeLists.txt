cmake_minimum_required(VERSION 3.20)
project(
  zebralpy
  VERSION 0.0.2
  LANGUAGES CXX
)

find_package(pybind11 CONFIG)

# This fixes a problem compiling on GitHub with Clang
# that I didn't see on Ubuntu 22.04
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-fsized-deallocation)
endif()

# pybind11 method:
pybind11_add_module(${PROJECT_NAME} zebralpy.cpp)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE zebralcommon::zebralcommon zebralserial::zebralserial
                          zebralcam::zebralcam
)


add_subdirectory(test)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
