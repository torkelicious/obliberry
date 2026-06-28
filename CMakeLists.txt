cmake_minimum_required(VERSION 4.2)
project(obliberry)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# External Sources
set(EXTERNAL_SOURCES
        # GLAD
        external/glad/src/glad.c
        # stb_image
        external/stb_image/stb_image.cpp
        # ImGui
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        #ImGuizmo
        external/imguizmo/ImGuizmo.cpp
        external/imguizmo/GraphEditor.cpp
        external/imguizmo/ImCurveEdit.cpp
        external/imguizmo/ImGradient.cpp
        external/imguizmo/ImSequencer.cpp
        external/imguizmo/ImVectorEditor.cpp
        # miniaudio
        external/miniaudio/miniaudio.c
)

# ObSL Core
set(OBSL_CORE_SOURCES
        src/Scripting/ObSLCore/Lexer/Lexer.cpp
        src/Scripting/ObSLCore/Parser/Parser.cpp
        src/Scripting/ObSLCore/Interpreter/Interpreter.cpp
        src/Scripting/ObSLCore/Interpreter/Environment.cpp
        src/Scripting/ObSLCore/ScriptEntry.cpp
        src/Scripting/ObSLCore/StdLib/StdLib.cpp
        src/Scripting/ObSLCore/GarbageCollector.cpp
)

# Engine Core Sources
# NOTE: ObSL EngineLib stay here, but the core language is removed, must be linked later!!!
set(ENGINE_SOURCES
        # Core & Window
        src/Core/Application.cpp
        src/Core/Window.cpp
        src/Core/InputManager.cpp
        src/Core/ProjectConfig.cpp
        src/Core/KeyMappings.cpp

        # Renderer
        src/Renderer/VertexBuffer.cpp
        src/Renderer/VertexArray.cpp
        src/Renderer/IndexBuffer.cpp
        src/Renderer/Renderer.cpp
        src/Renderer/Texture.cpp
        src/Renderer/Shader.cpp
        src/Renderer/GLDebug.cpp
        src/Renderer/MeshFactory.cpp

        # ECS / Scenes
        src/Scenes/Scene.cpp
        src/Game/GameLayer.cpp
        src/Game/GameUI.cpp

        # IO / Sound
        src/IO/MapSerialization.cpp
        src/IO/AssetLoader.cpp
        src/IO/EntityFactory.cpp
        src/IO/SceneSerialization.cpp
        src/Sound/AudioEngine.cpp

        # ObSL EngineLib
        src/Scripting/EngineLib/EngineLib.cpp
        src/Scripting/EngineLib/Modules/EngineLibAudio.cpp
        src/Scripting/EngineLib/Modules/EngineLibMap.cpp
        src/Scripting/EngineLib/Modules/EngineLibCore.cpp
        src/Scripting/EngineLib/Modules/EngineLibInput.cpp
        src/Scripting/EngineLib/Modules/EngineLibCamera.cpp
        src/Scripting/EngineLib/Modules/EngineLibSceneManagement.cpp
        src/Scripting/EngineLib/Modules/EngineLibTime.cpp
        src/IO/VFS.h
        src/IO/VFS.cpp
        src/Core/Project.cpp
        src/Core/Project.h
)

# ObSL Core
add_library(obsl_core STATIC ${OBSL_CORE_SOURCES})

# for linter
target_include_directories(obsl_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann
)

# Engine Library Target
add_library(obliberry_engine STATIC
        ${EXTERNAL_SOURCES}
        ${ENGINE_SOURCES}
)

target_include_directories(obliberry_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann
        ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link
target_link_libraries(obliberry_engine PUBLIC
        glfw
        glm::glm
        obsl_core
)

# Targets

# Runtime Executable
add_executable(obliberry_runtime src/Runtime/RuntimeMain.cpp)
target_link_libraries(obliberry_runtime PRIVATE obliberry_engine)

# Editor Executable
add_executable(obliberry_editor
        src/Editor/EditorMain.cpp
        src/Editor/EditorLayer.cpp
        src/Editor/EditorCamera.h
        src/Editor/UI/EditorWidgets.cpp
        src/Editor/UI/InspectorPanel.cpp
        src/Editor/UI/RegistryPanel.cpp
        src/Editor/UI/ViewportPanel.cpp
        src/Editor/UI/ProjectConfigEditor.cpp
        src/Editor/UI/ProjectConfigEditor.h
)

target_link_libraries(obliberry_editor PRIVATE obliberry_engine)

# Post build commands
#file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
# testing 4 now
#file(COPY ${CMAKE_SOURCE_DIR}/project.json DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/Templates DESTINATION ${CMAKE_BINARY_DIR})
