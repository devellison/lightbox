#!/bin/bash

set -e
pushd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_C_COMPILER=clang-12 -DCMAKE_CXX_COMPILER=clang++-12 -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --build build -t test
popd
