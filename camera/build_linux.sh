#!/bin/bash

set -e
pushd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_C_COMPILER=clang-12 -DCMAKE_CXX_COMPILER=clang++-12
cmake --build build --config Release
cmake --build build --config Release -t test
popd
