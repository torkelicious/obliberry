cmake_minimum_required(VERSION 3.23)

project(obliberry LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_PACK_TOOLS "Build .obpak packager tools" ON)

set(CONFIG_TYPES Debug Release RelWithDebInfo MinSizeRel)

# choose a cpu arch, default to a safe baseline
# x86-64-v3 requires ~Haswell/Zen1 or newer
set(ENGINE_ARCH_LEVEL "x86-64-v2" CACHE STRING "Target CPU architecture (x86-64-v2, x86-64-v3, native)")

# compiler / linker options
add_library(obliberry_options INTERFACE)

# meaningless (or error) on MSVC or ARM
if ((CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") AND (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686"))
    target_compile_options(obliberry_options INTERFACE $<$<NOT:$<CONFIG:Debug>>:-march=${ENGINE_ARCH_LEVEL}>)
    message(STATUS "Optimizing for CPU architecture level: ${ENGINE_ARCH_LEVEL}")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # split functions/data into sections & debug options
    target_compile_options(obliberry_options INTERFACE
            $<$<NOT:$<CONFIG:Debug>>:-ffunction-sections>
            $<$<NOT:$<CONFIG:Debug>>:-fdata-sections>
            $<$<CONFIG:RelWithDebInfo>:-fno-omit-frame-pointer>
            $<$<CONFIG:RelWithDebInfo>:-g>
    )
    # delete unused sections & linker optimizations
    target_link_options(obliberry_options INTERFACE
            $<$<NOT:$<CONFIG:Debug>>:-Wl,--gc-sections>
            $<$<CONFIG:RelWithDebInfo>:-Wl,--build-id>
            $<$<CONFIG:Release,MinSizeRel>:-s>
    )
elseif (MSVC)
    # MSVC/link.exe equivalents
    target_compile_options(obliberry_options INTERFACE $<$<NOT:$<CONFIG:Debug>>:/Gy>)
    target_link_options(obliberry_options INTERFACE
            $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
            $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>
    )
endif ()

# use a good linker where available :)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE)
    find_program(MOLD_LINKER mold)
    find_program(LLD_LINKER lld)
    if (MOLD_LINKER AND CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_link_options(obliberry_options INTERFACE "-fuse-ld=mold")
        message(STATUS "Using MOLD linker")
    elseif (LLD_LINKER)
        target_link_options(obliberry_options INTERFACE "-fuse-ld=lld")
        message(STATUS "Using LLD linker")
    endif ()
endif ()


if (APPLE)
    # min sdk to avoid issues for apple bros
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
endif ()

# god bless ccache
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "ccache found and enabled")
endif ()

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if (lto_supported)
    message(STATUS "LTO/IPO is supported and enabled for Release and RelWithDebInfo builds.")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
else ()
    message(WARNING "LTO/IPO is not supported by your compiler: ${error}")
endif ()

# Output dirs
set(OUTPUT_BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BIN_DIR}")

foreach (CONFIG ${CONFIG_TYPES})
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BIN_DIR}")
endforeach ()

# Helper
function(set_output_subdir SUBDIR)
    foreach (TGT ${ARGN})
        set_target_properties(${TGT} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BIN_DIR}/${SUBDIR}")
        foreach (CONFIG ${CONFIG_TYPES})
            string(TOUPPER "${CONFIG}" CONFIG_UPPER)
            set_target_properties(${TGT} PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BIN_DIR}/${SUBDIR}")
        endforeach ()
    endforeach ()
endfunction()

# Dependencies
include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.deps")

# Helper
function(fetch_dep NAME REPO TAG)
    FetchContent_Declare(
            ${NAME}
            GIT_REPOSITORY ${REPO}
            GIT_TAG ${TAG}
            GIT_SHALLOW TRUE
    )
endfunction()

fetch_dep(glfw https://github.com/glfw/glfw.git 3.4)
# disable GLFW extras to speed up build
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

fetch_dep(glm https://github.com/g-truc/glm.git 1.0.3)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

fetch_dep(nlohmann_json https://github.com/nlohmann/json.git v3.12.0)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

fetch_dep(nfd https://github.com/btzy/nativefiledialog-extended.git 3cd252a)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(NFD_PORTAL ON CACHE BOOL "Enable XDG Desktop Portal for Linux" FORCE)
endif ()
FetchContent_MakeAvailable(nfd)

fetch_dep(lz4 https://github.com/lz4/lz4.git v1.10.0)
set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lz4)

fetch_dep(freetype https://github.com/freetype/freetype.git VER-2-14-3)
set(FT_REQUIRE_ZLIB OFF CACHE BOOL "Disable ZLIB" FORCE)
set(FT_REQUIRE_BZIP2 OFF CACHE BOOL "Disable BZIP2" FORCE)
set(FT_REQUIRE_PNG OFF CACHE BOOL "Disable PNG" FORCE)
set(FT_REQUIRE_HARFBUZZ OFF CACHE BOOL "Disable HarfBuzz" FORCE)
set(FT_REQUIRE_BROTLI OFF CACHE BOOL "Disable Brotli" FORCE)
FetchContent_MakeAvailable(freetype)

# ObSL
set(OBSL_BUILD_RUNTIME OFF CACHE BOOL "" FORCE)
add_subdirectory(external/obsl)

add_library(obliberry_vendor STATIC
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
)

target_include_directories(obliberry_vendor SYSTEM PUBLIC
        external/glad/include
        external/imgui
        external/imgui/backends
        external/imguizmo
        external/stb_image
        external/miniaudio
)

target_link_libraries(obliberry_vendor PUBLIC glfw freetype)

# Engine target
add_library(obliberry_engine STATIC
        # Engine core
        src/Core/Application.cpp
        src/Platform/Window/Window.cpp
        src/Platform/Input/InputManager.cpp
        src/Config/ProjectConfig.cpp
        src/Platform/Input/KeyMappings.cpp
        src/Core/Project.cpp
        src/Platform/Threading/ThreadPool.cpp
        src/Logger/LoggerService.cpp

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
        src/Applications/Runtime/Game/GameLayer.cpp
        src/Applications/Runtime/Game/GameUI.cpp

        # IO
        src/IO/MapSerialization.cpp
        src/IO/Loaders/AssetLoader.cpp
        src/IO/Loaders/EntityFactory.cpp
        src/IO/SceneSerialization.cpp
        src/IO/VFS/VFS.cpp

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
        src/IO/Package/Tools/AssetPacking.cpp
        src/IO/Package/Tools/DependencyGraph.cpp
        src/Config/GraphicsConfig.cpp
        src/UI/Text/Font.cpp
        src/UI/Text/Font.h
        src/UI/Elements/UIText.cpp
)

target_include_directories(obliberry_engine PUBLIC
        src
)

target_compile_definitions(obliberry_engine PUBLIC
        $<$<CONFIG:Debug>:DEBUG_BUILD>
)

# link interface opts
target_link_libraries(obliberry_engine PUBLIC
        obliberry_options
        obliberry_vendor
        glm::glm
        obsl
        nlohmann_json::nlohmann_json
        nfd
        lz4
)

# Runtime
add_executable(obliberry_runtime
        src/Applications/Runtime/RuntimeMain.cpp
)

target_link_libraries(obliberry_runtime PRIVATE
        obliberry_engine
)

# Editor
add_executable(obliberry_editor
        src/Applications/Editor/EditorMain.cpp

        src/Applications/Editor/EditorLayer.cpp

        src/Applications/Editor/States/Editor/EditState.cpp
        src/Applications/Editor/States/Play/PlayState.cpp
        src/Applications/Editor/States/MapEditor/MapEditState.cpp
        src/Applications/Editor/States/Hub/HubState.cpp

        src/Applications/Editor/UI/Panels/Editor/EditorWidgets.cpp
        src/Applications/Editor/UI/Panels/Editor/EditorWidgetsCombo.cpp
        src/Applications/Editor/UI/Panels/Editor/InspectorPanel.cpp
        src/Applications/Editor/UI/Panels/ProjectBrowserPanel.cpp
        src/Applications/Editor/UI/Panels/Editor/RegistryPanel.cpp
        src/Applications/Editor/UI/Panels/ViewportPanel.cpp

        src/Applications/Editor/UI/ConfigWindows/ProjectConfigEditor.cpp
        src/Applications/Editor/UI/ConfigWindows/SceneConfigEditor.cpp

        src/Applications/Editor/Platform/FileDialogs.cpp

        src/IO/Package/Tools/ObpakTools.cpp

        src/Applications/Editor/UI/Panels/MapEditor/TileEditorPanel.cpp

        src/Applications/Editor/Commands/UndoManager.cpp
        src/Applications/Editor/Commands/EditorCommands.cpp

        src/Applications/Editor/UI/ConfigWindows/GraphicsConfigEditor.cpp
        src/Applications/Editor/UI/ConfigWindows/ConfigWindow.cpp
)

target_link_libraries(obliberry_editor PRIVATE
        obliberry_engine
)

option(ENABLE_UNITY_BUILD "Enable unity builds" ON)
if (ENABLE_UNITY_BUILD)
    set_target_properties(obliberry_engine obliberry_editor PROPERTIES UNITY_BUILD ON)
endif ()

# Tools
if (BUILD_PACK_TOOLS)

    add_executable(ob_packer
            src/Applications/Tools/Packaging/Packer.cpp
    )
    target_link_libraries(ob_packer PRIVATE obliberry_engine)

    add_executable(ob_unpacker
            src/Applications/Tools/Packaging/Unpacker.cpp
    )
    target_link_libraries(ob_unpacker PRIVATE obliberry_engine)

    add_executable(obsl_pack_run
            src/Applications/Tools/Packaging/Runner.cpp
    )
    target_link_libraries(obsl_pack_run PRIVATE obliberry_engine)

endif ()

# post build Template Sync
add_custom_command(
        TARGET obliberry_editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Templates"
        "${OUTPUT_BIN_DIR}/Templates"
        COMMENT "Copying templates to Editor output directory"
)

# output routing
set_output_subdir(internal obliberry_runtime)

if (BUILD_PACK_TOOLS)
    set_output_subdir(tools ob_packer ob_unpacker obsl_pack_run)
endif ()
