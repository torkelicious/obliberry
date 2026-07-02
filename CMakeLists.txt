cmake_minimum_required(VERSION 4.2)
project(obliberry)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch GLFW dep
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)
# disable GLFW extras to speed up build
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch GLM dep
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.3
)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

# Fetch nlohmann/json dep
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# Fetch nfd
FetchContent_Declare(
        nfd
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
        GIT_TAG master # master for proper glfw / wayland support, since releases are "slow"
)
# Use xdg portal rather on linux
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(NFD_PORTAL ON CACHE BOOL "Enable XDG Desktop Portal for Linux" FORCE)
endif ()

FetchContent_MakeAvailable(nfd)
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
        src/Scripting/ObSLCore/ScriptEntry.cpp
        src/Scripting/ObSLCore/Lexer/Lexer.cpp
        src/Scripting/ObSLCore/Parser/Parser.cpp
        src/Scripting/ObSLCore/Interpreter/Interpreter.cpp
        src/Scripting/ObSLCore/Interpreter/Environment.cpp
        src/Scripting/ObSLCore/StdLib/StdLib.cpp
        src/Scripting/ObSLCore/GarbageCollector.cpp
        src/Scripting/ObSLCore/ScriptWorker.cpp
        src/Scripting/ObSLCore/ScriptRuntime.cpp
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

        # Rendering
        src/Rendering/VertexBuffer.cpp
        src/Rendering/VertexArray.cpp
        src/Rendering/IndexBuffer.cpp
        src/Rendering/Renderer.cpp
        src/Rendering/Texture.cpp
        src/Rendering/Shader.cpp
        src/Rendering/GLDebug.cpp
        src/Rendering/MeshFactory.cpp

        # ECS / Scenes
        src/Scenes/Scene.cpp
        src/Scenes/SceneManager.cpp
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
        src/Scripting/EngineLib/Modules/EngineLibUI.cpp
        src/Scripting/EngineLib/ScriptCommandBuffer.cpp

        src/IO/VFS.cpp
        src/Core/Project.cpp
        src/Core/ThreadPool.cpp

)

# ObSL Core
add_library(obsl_core STATIC ${OBSL_CORE_SOURCES})

target_include_directories(obsl_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# JSON is only used internally by ObSL (linter output)
target_link_libraries(obsl_core PRIVATE
        nlohmann_json::nlohmann_json
)

# ObSL Core Runtime
add_executable(
        obsl_core_runtime
        src/Scripting/ObSLCore/ObSLCoreMain.cpp
)

target_link_libraries(obsl_core_runtime PRIVATE
        obsl_core
)

# Engine Library Target
add_library(obliberry_engine STATIC
        ${EXTERNAL_SOURCES}
        ${ENGINE_SOURCES}
)


target_include_directories(obliberry_engine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio
)


target_compile_definitions(obliberry_engine PUBLIC
        $<$<CONFIG:Debug>:DEBUG_BUILD>
)


target_link_libraries(obliberry_engine PUBLIC
        glfw
        glm::glm
        obsl_core
        nlohmann_json::nlohmann_json
        nfd
)


# Runtime Executable
add_executable(obliberry_runtime
        src/Runtime/RuntimeMain.cpp
)

target_link_libraries(obliberry_runtime PRIVATE
        obliberry_engine
)


# Editor Executable
add_executable(obliberry_editor
        src/Editor/EditorMain.cpp
        src/Editor/EditorLayer.cpp
        src/Editor/UI/Panels/EditorWidgets.cpp
        src/Editor/UI/Panels/InspectorPanel.cpp
        src/Editor/UI/Panels/RegistryPanel.cpp
        src/Editor/UI/Panels/ViewportPanel.cpp
        src/Editor/UI/ProjectConfigEditor.cpp
        src/Editor/FileDialogs.cpp
)

target_link_libraries(obliberry_editor PRIVATE
        obliberry_engine
)


# TEMP!!!!
#  = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
# MULTITHREADING TEST

# Thread Safety Test
add_executable(thread_safety_test src/Scripting/ObSLCore/threading_test.cpp)
target_link_libraries(thread_safety_test PRIVATE obliberry_engine)

# Copy test script to build dir
file(COPY ${CMAKE_SOURCE_DIR}/src/Scripting/ObSLCore/Examples/test.obsl
        DESTINATION ${CMAKE_BINARY_DIR})

# Set cwd
set_target_properties(thread_safety_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# MULTITHREADING TEST
#  = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

# Post build commands
file(COPY ${CMAKE_SOURCE_DIR}/Templates DESTINATION ${CMAKE_BINARY_DIR})
