@echo off
REM This script just cleans up the mess from a build

REM Save current directory, but move to script directory
pushd "%~dp0"

REM Remove all build files if build directory exists.
if exist ..\build rd ..\build /s /q
popd