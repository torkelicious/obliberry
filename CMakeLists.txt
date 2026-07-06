cmake_minimum_required(VERSION 3.20)

project(obliberry LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_PACK_TOOLS "Build .obpack packager tools" ON)

# Output dirs
set(OUTPUT_BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BIN_DIR}")

foreach (CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BIN_DIR}")
endforeach ()

# Dependencies
include(FetchContent)

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

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.3
)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
        nfd
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
        GIT_TAG master # master for proper glfw / wayland support, since releases are "slow"
)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(NFD_PORTAL ON CACHE BOOL "Enable XDG Desktop Portal for Linux" FORCE)
    set(NFD_PORTAL ON CACHE BOOL "" FORCE)
endif ()
FetchContent_MakeAvailable(nfd)

FetchContent_Declare(
        lz4
        GIT_REPOSITORY https://github.com/lz4/lz4.git
        GIT_TAG v1.10.0
)
FetchContent_MakeAvailable(lz4)

# ObSL
set(OBSL_BUILD_RUNTIME OFF CACHE BOOL "" FORCE)
add_subdirectory(external/obsl)

# Engine target
add_library(obliberry_engine STATIC

        # External sources

        # GLAD
        external/glad/src/glad.c
        external/stb_image/stb_image.cpp

        # ImGui
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp

        # ImGuizmo
        external/imguizmo/ImGuizmo.cpp
        external/imguizmo/GraphEditor.cpp
        external/imguizmo/ImCurveEdit.cpp
        external/imguizmo/ImGradient.cpp
        external/imguizmo/ImSequencer.cpp
        external/imguizmo/ImVectorEditor.cpp
        # Miniaudio
        external/miniaudio/miniaudio.c

        # Engine core
        src/Core/Application.cpp
        src/Core/Window.cpp
        src/Core/InputManager.cpp
        src/Core/ProjectConfig.cpp
        src/Core/KeyMappings.cpp
        src/Core/Project.cpp
        src/Core/ThreadPool.cpp

        # Rendering
        src/Rendering/VertexBuffer.cpp
        src/Rendering/VertexArray.cpp
        src/Rendering/IndexBuffer.cpp
        src/Rendering/Renderer.cpp
        src/Rendering/Texture.cpp
        src/Rendering/Shader.cpp
        src/Rendering/GLDebug.cpp
        src/Rendering/MeshFactory.cpp

        # Scenes / gameplay
        src/Scenes/Scene.cpp
        src/Scenes/SceneManager.cpp
        src/Game/GameLayer.cpp
        src/Game/GameUI.cpp

        # IO
        src/IO/MapSerialization.cpp
        src/IO/AssetLoader.cpp
        src/IO/EntityFactory.cpp
        src/IO/SceneSerialization.cpp
        src/IO/VFS.cpp

        # Sound
        src/Sound/AudioEngine.cpp

        # Scripting / EngineLib
        src/Scripting/EngineLib/EngineLib.cpp
        src/Scripting/EngineLib/ScriptCommandBuffer.cpp

        src/Scripting/EngineLib/Modules/EngineLibAudio.cpp
        src/Scripting/EngineLib/Modules/EngineLibMap.cpp
        src/Scripting/EngineLib/Modules/EngineLibCore.cpp
        src/Scripting/EngineLib/Modules/EngineLibInput.cpp
        src/Scripting/EngineLib/Modules/EngineLibCamera.cpp
        src/Scripting/EngineLib/Modules/EngineLibSceneManagement.cpp
        src/Scripting/EngineLib/Modules/EngineLibTime.cpp
        src/Scripting/EngineLib/Modules/EngineLibUI.cpp

        # Packaging
        src/IO/Package/ContainerWriter.cpp
        src/IO/Package/ContainerReader.cpp
        src/IO/Package/Tools/CliCommon.cpp
)

target_include_directories(obliberry_engine PUBLIC
        src

        external/glad/include
        external/imgui
        external/imgui/backends
        external/imguizmo
        external/stb_image
        PRIVATE
        external/miniaudio
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

# Runtime
add_executable(obliberry_runtime
        src/Runtime/RuntimeMain.cpp
)

target_link_libraries(obliberry_runtime PRIVATE
        obliberry_engine
)

# Editor
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
        src/Editor/UI/ConfigWindows/SceneConfigEditor.cpp
        src/Editor/FileDialogs.cpp
)

target_link_libraries(obliberry_editor PRIVATE
        obliberry_engine
)

# Tools
if (BUILD_PACK_TOOLS)

    add_executable(ob_packer
            src/IO/Package/Packer.cpp
            src/IO/Package/Tools/AssetPacking.cpp
            src/IO/Package/Tools/DependencyGraph.cpp
    )
    target_link_libraries(ob_packer PRIVATE obliberry_engine)

    add_executable(ob_unpacker
            src/IO/Package/Unpacker.cpp
    )
    target_link_libraries(ob_unpacker PRIVATE obliberry_engine)

    add_executable(obsl_pack_run
            src/IO/Package/Runner.cpp
    )
    target_link_libraries(obsl_pack_run PRIVATE obliberry_engine)

endif ()

# Post build
file(COPY "${CMAKE_SOURCE_DIR}/Templates" DESTINATION "${OUTPUT_BIN_DIR}")
