cmake_minimum_required(VERSION 4.2)
project(obliberry)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_PACK_TOOLS "Build .obpack packager tools" ON)

set(OUTPUT_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
# place all executables in build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR})
foreach (CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${OUTPUT_BIN_DIR})
endforeach ()

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

# Fetch LZ4
FetchContent_Declare(
        lz4
        GIT_REPOSITORY https://github.com/lz4/lz4.git
        GIT_TAG v1.10.0
)
FetchContent_MakeAvailable(lz4)


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

# ObSL
set(OBSL_BUILD_RUNTIME OFF CACHE BOOL "" FORCE)
add_subdirectory(external/obsl)


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
        src/IO/Package/ContainerWriter.cpp
        src/IO/Package/ContainerReader.cpp
        src/IO/Package/Tools/CliCommon.h
        src/IO/Package/Tools/CliCommon.cpp
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
        obsl
        nlohmann_json::nlohmann_json
        nfd
        lz4
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
        src/Editor/States/EditState.cpp
        src/Editor/States/PlayState.cpp
        src/Editor/States/MapEditState.cpp
        src/Editor/UI/Panels/EditorWidgets.cpp
        src/Editor/UI/Panels/InspectorPanel.cpp
        src/Editor/UI/Panels/RegistryPanel.cpp
        src/Editor/UI/Panels/ViewportPanel.cpp
        src/Editor/UI/ConfigWindows/ProjectConfigEditor.cpp
        src/Editor/FileDialogs.cpp
        src/Editor/UI/ConfigWindows/SceneConfigEditor.cpp
)

target_link_libraries(obliberry_editor PRIVATE
        obliberry_engine
)

# Optional Tools
if (BUILD_PACK_TOOLS)
    # Packager Tool
    add_executable(ob_packer
            src/IO/Package/Packer.cpp
            src/IO/Package/Tools/AssetPacking.cpp
            src/IO/Package/Tools/DependencyGraph.cpp
    )
    target_link_libraries(ob_packer PRIVATE obliberry_engine)

    # Unpacker tool
    add_executable(ob_unpacker src/IO/Package/Unpacker.cpp)
    target_link_libraries(ob_unpacker PRIVATE obliberry_engine)

    # Runner Tool (runs script inside package from its premade AST
    add_executable(obsl_pack_run src/IO/Package/Runner.cpp)
    target_link_libraries(obsl_pack_run PRIVATE obliberry_engine)
endif ()

# Post build commands
file(COPY ${CMAKE_SOURCE_DIR}/Templates DESTINATION ${OUTPUT_BIN_DIR})
