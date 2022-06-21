#!/bin/bash
set -e
pushd "$(dirname "$0")" 
echo "Formatting Code...."
clang-format -i app/src/*.cpp
clang-format -i app/inc/*.hpp
clang-format -i test/*.cpp

clang-format -i zebral/camera/src/*.cpp
clang-format -i zebral/camera/inc/*.hpp
clang-format -i zebral/camera/test/*.cpp

clang-format -i zebral/common/src/*.cpp
clang-format -i zebral/common/inc/*.hpp
clang-format -i zebral/common/test/*.cpp

clang-format -i zebral/serial/src/*.cpp
clang-format -i zebral/serial/inc/*.hpp
clang-format -i zebral/serial/test/*.cpp

echo "Formatting CMake..."
cmake-format -i CMakeLists.txt
cmake-format -i app/CMakeLists.txt
cmake-format -i test/CMakeLists.txt

cmake-format -i zebral/common/CMakeLists.txt
cmake-format -i zebral/common/test/CMakeLists.txt

cmake-format -i zebral/camera/CMakeLists.txt
cmake-format -i zebral/camera/test/CMakeLists.txt

cmake-format -i zebral/serial/CMakeLists.txt
cmake-format -i zebral/serial/test/CMakeLists.txt

popd
