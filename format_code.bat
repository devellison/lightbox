@echo off

pushd "%~dp0"
echo Formatting Code....
clang-format -i app\src\*.cpp || goto error
clang-format -i app\inc\*.hpp || goto error
clang-format -i test\*.cpp || goto error

clang-format -i zebral\camera\src\*.cpp || goto error
clang-format -i zebral\camera\inc\*.hpp || goto error
clang-format -i zebral\camera\test\*.cpp || goto error

clang-format -i zebral\common\src\*.cpp || goto error
clang-format -i zebral\common\inc\*.hpp || goto error
clang-format -i zebral\common\test\*.cpp || goto error

clang-format -i zebral\serial\src\*.cpp || goto error
clang-format -i zebral\serial\inc\*.hpp || goto error
clang-format -i zebral\serial\test\*.cpp || goto error

echo Formatting CMake...

cmake-format -i CMakeLists.txt  || goto error
cmake-format -i app\CMakeLists.txt || goto error
cmake-format -i test\CMakeLists.txt || goto error

cmake-format -i zebral\camera\CMakeLists.txt || goto error
cmake-format -i zebral\camera\test\CMakeLists.txt || goto error

cmake-format -i zebral\common\CMakeLists.txt || goto error
cmake-format -i zebral\common\test\CMakeLists.txt || goto error

cmake-format -i zebral\serial\CMakeLists.txt || goto error
cmake-format -i zebral\serial\test\CMakeLists.txt || goto error

echo "Reformatted everything."
exit /b 0
popd
:error
echo "An error occurred!"
popd
exit /b 1


