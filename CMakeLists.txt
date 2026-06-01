cmake_minimum_required(VERSION 4.2)
project(IsometricGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(glfw3 REQUIRED)
add_executable(IsometricGame
        glad/src/glad.c
        src/main.cpp)

target_include_directories(
        IsometricGame
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)

target_link_libraries(IsometricGame glfw)
