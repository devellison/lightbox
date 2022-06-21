# ZebralSerial library Simple serial comms
project(
  zebralserial
  VERSION 0.0.2
  LANGUAGES CXX
)

# Add serial library
set(SRC src/serial_channel.cpp src/serial_linux.cpp src/serial_winrt.cpp
        src/serial_info.cpp
)

set(INC inc/serial_channel.hpp inc/serial_info.hpp)

add_library(${PROJECT_NAME} ${SRC} ${INC})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(
  ${PROJECT_NAME} PUBLIC inc ${fmt_INCLUDE_DIRS} zebralcam::zebralcam
)
target_link_libraries(
  ${PROJECT_NAME} PUBLIC ${EXTRA_LIBS} Threads::Threads zebralcam::zebralcam
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

add_subdirectory(test)

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
