cmake_minimum_required(VERSION 3.20)

# Basic unit tests
add_executable(basic_tests basic_tests.cpp)
target_link_libraries(basic_tests gtest_main zebralcam)
gtest_add_tests(TARGET basic_tests)
