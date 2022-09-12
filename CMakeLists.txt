cmake_minimum_required(VERSION 3.14)
project(chip8-vm VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 11)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

find_package(SDL2 REQUIRED)

add_executable(
    chip8-vm
    Source/main.cpp
    Source/chip8.cpp
    Source/platform.cpp)

target_compile_options(chip8-vm PRIVATE -Wall)

target_link_libraries(chip8-vm PRIVATE SDL2::SDL2)