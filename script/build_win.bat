@echo off

REM This script runs cmake and builds the lightbox app with msbuild.

REM Save current directory, but move to script directory
pushd "%~dp0"
mkdir ..\build
cd ..\build
cmake ..
REM Might make this an argument for debug...
msbuild ALL_BUILD.vcxproj /p:Configuration=Release
popd

