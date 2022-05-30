@ECHO off
REM Configure, build, and test on windows

SET /A package=0
SET /A test=0
SET /A docs=0

IF [%1] == [test] GOTO test
IF [%1] == [package] GOTO package
IF [%1] == [Debug] GOTO setfromarg
IF [%1] == [Release] GOTO setfromarg
IF [%1] == [RelWithDebInfo] GOTO setfromarg
IF [%1] == [] GOTO defaultbuild

:usage
ECHO Usage: build_win.bat BUILDTYPE
ECHO   build_win.bat Debug
ECHO   build_win.bat Release
ECHO   build_win.bat RelWithDebInfo
ECHO   build_win.bat test
ECHO   build_win.bat package
EXIT /b 4

:package
SET /A package=1
SET /A test=1
SET /A docs=1
GOTO configure

:test
SET buildtype=Release
SET /A test=1
GOTO configure

:defaultbuild
SET buildtype=Release
GOTO configure

:setfromarg
SET buildtype=%1
:configure
pushd "%~dp0"

REM cmake-gui -S . -B build || GOTO gen_error
cmake -S . -B build || GOTO gen_error
cmake --build build --config %buildtype% -j8 || GOTO build_error
IF %test%==1 cmake --build build --config %buildtype% -t RUN_TESTS || GOTO test_error
IF %package%==1 cmake --build build --config %buildtype% -t package || GOTO package_error
:success
popd
EXIT /b 0

:package_error
ECHO Error generating package (5)
popd
EXIT /b 5

:gen_error
ECHO Error generating makefile (1)
popd
EXIT /b 1

:build_error
ECHO Error building (2)
popd
EXIT /b 2

:test_error
ECHO Error in unit tests (3)
popd
EXIT /b 3

