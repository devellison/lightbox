@echo off

pushd "%~dp0"
echo Formatting Code....
clang-format -i app\src\*.cpp || goto error
clang-format -i test\*.cpp || goto error
clang-format -i camera\src\*.cpp || goto error
clang-format -i camera\test\*.cpp || goto error
clang-format -i app\inc\*.hpp || goto error
clang-format -i camera\inc\*.hpp || goto error
echo Formatting CMake...
cmake-format -i CMakeLists.txt  || goto error
cmake-format -i app\CMakeLists.txt || goto error
cmake-format -i test\CMakeLists.txt || goto error
cmake-format -i camera\CMakeLists.txt || goto error

echo "Reformatted everything."
exit /b 0
popd
:error
echo "An error occurred!"
popd
exit /b 1


