cmake_minimum_required(VERSION 3.20)

# Basic unit tests
add_executable(serial_test serial_test.cpp)
target_link_libraries(serial_test gtest_main zebralcam zebralserial)
gtest_add_tests(TARGET serial_test)
