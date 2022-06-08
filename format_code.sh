#!/bin/bash
set -e
pushd "${0%/*}"
echo "Formatting Code...."
clang-format -i app/src/*.cpp
clang-format -i test/*.cpp
clang-format -i camera/src/*.cpp
clang-format -i camera/test/*.cpp
clang-format -i app/inc/*.hpp
clang-format -i camera/inc/*.hpp
echo "Formatting CMake..."
cmake-format -i CMakeLists.txt
cmake-format -i app/CMakeLists.txt
cmake-format -i test/CMakeLists.txt
cmake-format -i camera/CMakeLists.txt
popd
