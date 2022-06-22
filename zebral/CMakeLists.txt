# Zebral libraries
cmake_minimum_required(VERSION 3.20)

# Each of these makes a static library that can be individually included.
add_subdirectory(common)
add_subdirectory(camera)
add_subdirectory(serial)

# But it might be nice to have one big shared library of it all...
project(
  zebral
  VERSION 0.0.2
  LANGUAGES CXX
)

include(GenerateExportHeader)

add_library(${PROJECT_NAME} SHARED src/zebral.cpp)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(
  ${PROJECT_NAME} PUBLIC inc ${fmt_INCLUDE_DIRS} zebralcommon::zebralcommon
                         zebralcam::zebralcam zebralserial::zebralserial
)
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC ${EXTRA_LIBS} Threads::Threads zebralcommon::zebralcommon
         zebralcam::zebralcam zebralserial::zebralserial
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_definitions(
  ${PROJECT_NAME} PUBLIC LIGHTBOX_VERSION="${PROJECT_VERSION}"
)

generate_export_header(zebral)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${INC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(test)
