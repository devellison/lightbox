#!/bin/bash

configure()
{
    cmake -S . -B build -DCMAKE_C_COMPILER=clang-12 -DCMAKE_CXX_COMPILER=clang++-12 -DCMAKE_BUILD_TYPE=$BuildType
}
build()
{
    cmake --build build
}
test()
{
    cmake --build build -t test
}

pack()
{
    cmake --build build -t lightbox_docs
    cd build
    cpack
}

#----------------------------------------
#
pushd "$(dirname "$0")"
set -e

BuildType="RelWithDebInfo"
Test=0
Pack=0

if [ "$1" == "Debug" ]; then
    BuildType=$1
elif [ "$1" == "Release" ]; then
    BuildType="RelWithDebInfo"
elif [ "$1" == "test" ]; then
    BuildType="RelWithDebInfo"
    Test=1
elif [ "$1" == "package" ]; then
    BuildType="RelWithDebInfo"
    Test=1
    Pack=1
fi

configure
build
if [ "$Test" -gt "0" ]; then
    test
fi
if [ "$Pack" -gt "0" ]; then
    pack
fi
popd
