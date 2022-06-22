cmake_minimum_required(VERSION 3.20)

find_package(
  Python
  COMPONENTS Interpreter
  REQUIRED
)
add_test(NAME zebral_python_test
         COMMAND ${Python_EXECUTABLE}
                 ${CMAKE_CURRENT_SOURCE_DIR}/zebral_python_test.py
)
