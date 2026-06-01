cmake_minimum_required(VERSION 4.2)
project(IsometricGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# i have GLM and GLFW handeled as system packages via pacman for now.

add_executable(
        IsometricGame
        src/main.cpp
        external/glad/src/glad.c
)

target_include_directories(
        IsometricGame PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
        IsometricGame 
        glfw
)
