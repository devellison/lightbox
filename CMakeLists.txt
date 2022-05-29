cmake_minimum_required(VERSION 3.15)
project(lightbox LANGUAGES CXX)

# High warning level, treat warnings as errors, compile as a console with main().
if (MSVC)
    add_compile_options(/W4 /WX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_executable(${PROJECT_NAME} app/lightbox.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC app)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
