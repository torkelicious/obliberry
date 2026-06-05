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
        external/stb_image/stb_image.cpp
        src/Core/Window.cpp
        src/Core/Window.h
        src/Core/Application.cpp
        src/Core/Application.h
        src/Graphics/Renderer.cpp
        src/Graphics/Renderer.h
        src/Graphics/Mesh.cpp
        src/Graphics/Mesh.h
        src/Graphics/Shader.cpp
        src/Graphics/Shader.h
        src/Graphics/MeshFactory.cpp
        src/Graphics/MeshFactory.h
        src/Graphics/Texture.cpp
        src/Graphics/Texture.h
        src/Graphics/Camera.cpp
        src/Graphics/Camera.h
        src/Map/HexMap.cpp
        src/Map/HexMap.h
        src/Graphics/GLDebug.cpp
        src/Graphics/GLDebug.h
        src/Graphics/Material.h
        src/Core/InputManager.cpp
        src/Core/InputManager.h
        src/Game/GameWorld.cpp
        src/Game/GameWorld.h
        src/ECS/ECS.h
        src/ECS/ECS.cpp
        src/ECS/Components.h
        src/ECS/Components/TransformComponent.h
        src/ECS/Components/PlayerInputComponent.h
        src/ECS/Components/VelocityComponent.h
        src/ECS/Components/SpriteComponent.h
        src/ECS/Systems/InputSystem.h
        src/ECS/Components/SpriteComponent.cpp
        src/ECS/Components/MeshComponent.h
        src/ECS/Components/MaterialComponent.h
        src/ECS/Components/HexPositionComponent.h
        src/Core/ResourceManager.cpp
        src/Core/ResourceManager.h
)

target_include_directories(
        IsometricGame PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
        IsometricGame
        glfw
)
