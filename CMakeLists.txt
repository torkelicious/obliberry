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
        # nlohmann/json
        external/nlohmann/json.hpp
        external/nlohmann/json_fwd.hpp
        # miniaudio
        external/miniaudio/miniaudio.h
        external/miniaudio/miniaudio.c
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
        src/Renderer/GLDebug.cpp
        src/Renderer/GLDebug.h
        src/Map/Hex.h
        src/Renderer/Camera.h
        src/ECS/ECS.h
        src/ECS/Components/MaterialComponent.h
        src/ECS/Components/MeshComponent.h
        src/Core/ResourceManager.h
        src/Game/Game.cpp
        src/Game/Game.h
        src/Game/GameUI.cpp
        src/Core/Constants.h
        src/Renderer/MeshFactory.cpp
        src/Renderer/MeshFactory.h
        src/Core/InputManager.cpp
        src/Core/InputManager.h
        src/ECS/Components/TransformComponent.h
        src/ECS/Systems/MovementSystem.h
        src/Renderer/Transform.h
        src/Math/HexMath.h
        src/Map/HexCoords.h
        src/IO/MapSerialization.cpp
        src/IO/MapSerialization.h
        src/ECS/Systems/RenderSystem.h
        src/ECS/Types.h
        src/ECS/IPool.h
        src/ECS/ComponentPool.h
        src/ECS/Registry.h
        src/ECS/Entity.h
        src/Scenes/Scene.h
        src/Core/Utils.h
        src/Scenes/SceneManager.h
        src/Scenes/Scene.cpp
        src/Core/EngineContext.h
        src/ECS/Systems/InteractionSystem.h
        src/ECS/Systems/PlayerControlSystem.h
        src/ECS/Systems/AISystem.h
        src/ECS/Components/MovementComponent.h
        src/ECS/Systems/SpriteBillboardSystem.h
        src/ECS/Systems/MapRenderSystem.h
        src/ECS/Components/BillboardTagComponent.h
        src/ECS/Systems/DirectionalAnimationSystem.h
        src/ECS/Systems/MapRuntimeSystem.h
        src/IO/AssetLoader.cpp
        src/IO/AssetLoader.h
        src/ECS/Components/DirectionalTextureComponent.h
        src/IO/EntityFactory.cpp
        src/IO/EntityFactory.h
        src/ECS/Components/MapComponent.h
        src/ECS/Components/MapStateComponent.h
        src/IO/SceneSerialization.cpp
        src/IO/SceneSerialization.h
        src/Math/Math.h
        src/ECS/Systems/lightingSystem.h
        src/ECS/Components/PointLightComponent.h
        # ObSL Core
        src/Scripting/ObSLCore/Lexer/Lexer.cpp
        src/Scripting/ObSLCore/Lexer/Lexer.h
        src/Scripting/ObSLCore/Parser/ast.h
        src/Scripting/ObSLCore/Parser/Parser.cpp
        src/Scripting/ObSLCore/Parser/Parser.h
        src/Scripting/ObSLCore/Tokens.h
        src/Scripting/ObSLCore/Interpreter/Interpreter.cpp
        src/Scripting/ObSLCore/Interpreter/Interpreter.h
        src/Scripting/ObSLCore/Interpreter/Environment.cpp
        src/Scripting/ObSLCore/Interpreter/Environment.h
        src/Scripting/ObSLCore/ScriptEntry.cpp
        src/Scripting/ObSLCore/ScriptEntry.h
        src/Scripting/ObSLCore/StdLib/StdLib.cpp
        src/Scripting/ObSLCore/StdLib/StdLib.h
        src/Scripting/ObSLCore/StdLib/StdModules.h
        src/Scripting/ObSLCore/GarbageCollector.cpp
        src/Scripting/ObSLCore/GarbageCollector.h
        src/Scripting/ObSLCore/Interpreter/Natives.h
        # ObSL Core
        src/ECS/Components/ScriptComponent.h
        src/ECS/Systems/ScriptSystem.h
        src/Scripting/EngineLib/EngineLib.cpp
        src/Scripting/EngineLib/EngineLib.h
        src/ECS/Components/DestroyTagComponent.h
        src/Scripting/EngineLib/EngineLibFactories.h
        src/IO/PrefabManager.h
        src/Sound/AudioEngine.cpp
        src/Sound/AudioEngine.h
        src/Scripting/EngineLib/Modules/EngineLibAudio.cpp
        src/Scripting/EngineLib/Modules/EngineLibMap.cpp
        src/Scripting/EngineLib/Modules/EngineLibCore.cpp
        src/Scripting/EngineLib/Modules/EngineLibInput.cpp
        src/Scripting/EngineLib/Modules/EngineLibCamera.cpp
        src/Scripting/EngineLib/Modules/EngineLibSceneManagement.cpp
)

target_include_directories(
        obliberry PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann/
        ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio/
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
        obliberry
        glfw
)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
