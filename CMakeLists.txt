cmake_minimum_required(VERSION 4.2)
project(obliberry)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(
        obliberry

        # External
        # GLAD
        external/glad/src/glad.c
        # stb_image
        external/stb_image/stb_image.cpp
        # imgui
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_glfw.h
        external/imgui/backends/imgui_impl_opengl3.cpp
        external/imgui/backends/imgui_impl_opengl3.h
        external/imgui/backends/imgui_impl_opengl3_loader.h
        external/imgui/imconfig.h
        external/imgui/imgui.cpp
        external/imgui/imgui.h
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_internal.h
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imstb_rectpack.h
        external/imgui/imstb_textedit.h
        external/imgui/imstb_truetype.h
        # imgui
        # External

        src/main.cpp
        src/Core/Application.cpp
        src/Core/Application.h
        src/Core/Window.cpp
        src/Core/Window.h
        src/Renderer/VertexBuffer.cpp
        src/Renderer/VertexBuffer.h
        src/Renderer/VertexArray.cpp
        src/Renderer/VertexArray.h
        src/Renderer/IndexBuffer.cpp
        src/Renderer/IndexBuffer.h
        src/Renderer/VertexBufferLayout.h
        src/Renderer/Renderer.cpp
        src/Renderer/Renderer.h
        src/Renderer/Mesh.h
        src/Renderer/Texture.cpp
        src/Renderer/Texture.h
        src/Renderer/Material.h
        src/Renderer/Shader.cpp
        src/Renderer/Shader.h
        src/Renderer/Material.cpp
        src/Renderer/GLDebug.cpp
        src/Renderer/GLDebug.h
        src/Map/Hex.h
        src/Renderer/Camera.h
        src/ECS/ECS.h
        src/ECS/Components/MaterialComponent.h
        src/ECS/Components/MeshComponent.h
        src/ECS/Components/VelocityComponent.h
        src/Core/ResourceManager.h
        src/Game/Game.cpp
        src/Game/Game.h
        src/Core/Constants.h
        src/Renderer/MeshFactory.cpp
        src/Renderer/MeshFactory.h
        src/ECS/Systems/PlayerInput.h
        src/Core/InputManager.cpp
        src/Core/InputManager.h
        src/ECS/Components/TransformComponent.h
        src/ECS/Systems/Movement.h
        src/Renderer/Transform.h
)


target_include_directories(
        obliberry PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_link_libraries(
        obliberry
        glfw
)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})