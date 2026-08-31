# Engine Target
add_library(obliberry_engine STATIC
        # Engine core
        Core/Application.cpp
        Platform/Window/Window.cpp
        Platform/Input/InputManager.cpp
        Config/ProjectConfig.cpp
        Config/GraphicsConfig.cpp
        Platform/Input/KeyMappings.cpp
        Core/Project.cpp
        Platform/Threading/ThreadPool.cpp
        Logger/LoggerService.cpp

        # Rendering
        Rendering/Types/VBO/VertexBuffer.cpp
        Rendering/Types/VAO/VertexArray.cpp
        Rendering/Types/IBO/IndexBuffer.cpp
        Rendering/Renderer.cpp
        Rendering/Types/Texture/Texture.cpp
        Rendering/Types/Shader/Shader.cpp
        Rendering/GLDebug.cpp
        Rendering/Types/Mesh/MeshFactory.cpp
        Rendering/PostProcessing/PostProcessing.cpp

        # Scenes / gameplay
        Scenes/Scene.cpp
        Scenes/SceneManager.cpp
        Applications/Runtime/Game/GameLayer.cpp
        Applications/Runtime/Game/GameUI.cpp

        # IO
        IO/MapSerialization.cpp
        IO/Loaders/AssetLoader.cpp
        IO/Loaders/EntityFactory.cpp
        IO/Loaders/UISerializer.cpp
        IO/SceneSerialization.cpp
        IO/VFS/VFS.cpp

        # Sound
        Sound/AudioEngine.cpp

        # Scripting / EngineLib
        Scripting/EngineLib/EngineLib.cpp
        Scripting/EngineLib/ScriptCommandBuffer.cpp
        Scripting/EngineLib/Modules/EngineLibAudio.cpp
        Scripting/EngineLib/Modules/EngineLibMap.cpp
        Scripting/EngineLib/Modules/EngineLibCore.cpp
        Scripting/EngineLib/Modules/EngineLibInput.cpp
        Scripting/EngineLib/Modules/EngineLibCamera.cpp
        Scripting/EngineLib/Modules/EngineLibSceneManagement.cpp
        Scripting/EngineLib/Modules/EngineLibTime.cpp
        Scripting/EngineLib/Modules/EngineLibUI.cpp
        Scripting/EngineLib/Modules/EngineLibRegistry.cpp

        # Packaging
        IO/Package/ContainerWriter.cpp
        IO/Package/ContainerReader.cpp
        IO/Package/Tools/AssetPacking.cpp
        IO/Package/Tools/DependencyGraph.cpp
        IO/Package/Tools/IgnoreRules.cpp

        # UI
        UI/Text/Font.cpp
        UI/Text/Font.h
        UI/Elements/UIText.cpp
        UI/Elements/UIButton.cpp
        UI/Elements/UIImage.cpp
        UI/Elements/UIRect.cpp
        UI/Rendering/UIRenderer.cpp
        UI/Rendering/UISystem.h
        UI/Rendering/UISystem.cpp
)

target_include_directories(obliberry_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(obliberry_engine PUBLIC
        $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_link_libraries(obliberry_engine PUBLIC
        obliberry_options
        obliberry_deps
        obsl
)

# Runtime Executable
add_executable(obliberry_runtime
        Applications/Runtime/RuntimeMain.cpp
)
target_link_libraries(obliberry_runtime PRIVATE obliberry_engine)

# Editor Executable
add_executable(obliberry_editor
        # application
        Applications/Editor/EditorMain.cpp
        Applications/Editor/EditorLayer.cpp

        # states
        Applications/Editor/States/Hub/HubState.cpp
        Applications/Editor/States/Editor/EditState.cpp
        Applications/Editor/States/Play/PlayState.cpp
        Applications/Editor/States/MapEditor/MapEditState.cpp

        # ui
        Applications/Editor/UI/Panels/Editor/EditorWidgetsCombo.cpp
        Applications/Editor/UI/Panels/Editor/EditorWidgets.cpp

        # panels
        Applications/Editor/UI/Panels/Editor/InspectorPanel.cpp
        Applications/Editor/UI/Panels/ViewportPanel.cpp
        Applications/Editor/UI/Panels/Editor/RegistryPanel.cpp
        Applications/Editor/UI/Panels/ProjectBrowserPanel.cpp
        Applications/Editor/UI/Panels/MapEditor/TileEditorPanel.cpp
        Applications/Editor/UI/Panels/Editor/UIPanel.cpp
        Applications/Editor/UI/Panels/Editor/MeshCreatorPanel.cpp
        Applications/Editor/UI/ConfigWindows/PostProcEditor.cpp

        # config windows
        Applications/Editor/UI/ConfigWindows/ConfigWindow.cpp
        Applications/Editor/UI/ConfigWindows/ThemeConfigEditor.cpp
        Applications/Editor/UI/ConfigWindows/ProjectConfigEditor.cpp
        Applications/Editor/UI/ConfigWindows/SceneConfigEditor.cpp
        Applications/Editor/UI/ConfigWindows/GraphicsConfigEditor.cpp

        # platform
        Applications/Editor/Platform/FileDialogs.cpp
        IO/Package/Tools/ObpakTools.cpp

        # undo-redo stack
        Applications/Editor/Commands/UndoManager.cpp
        Applications/Editor/Commands/EditorCommands.cpp
)

target_link_libraries(obliberry_editor PRIVATE obliberry_engine)

# Unity Build Handling
if (ENABLE_UNITY_BUILD)
    set_target_properties(obliberry_engine obliberry_editor PROPERTIES UNITY_BUILD ON)
endif ()

# Packaging Tools Target
if (BUILD_PACK_TOOLS)
    add_executable(ob_packer Applications/Tools/Packaging/Packer.cpp)
    target_link_libraries(ob_packer PRIVATE obliberry_engine)

    add_executable(ob_unpacker Applications/Tools/Packaging/Unpacker.cpp)
    target_link_libraries(ob_unpacker PRIVATE obliberry_engine)

    add_executable(obsl_pack_run Applications/Tools/Packaging/Runner.cpp)
    target_link_libraries(obsl_pack_run PRIVATE obliberry_engine)
endif ()

# Internal resources
add_custom_command(
        TARGET obliberry_editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "${OUTPUT_BIN_DIR}/internal/resources"
        COMMENT "Copying internal resources to Editor internal/ output directory"
)

# Post-build Project Template
add_custom_command(
        TARGET obliberry_editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Templates"
        "${OUTPUT_BIN_DIR}/Templates"
        COMMENT "Copying templates to Editor output directory"
)

# Output Routing
set_output_subdir(internal obliberry_runtime)

if (BUILD_PACK_TOOLS)
    set_output_subdir(tools ob_packer ob_unpacker obsl_pack_run)
endif ()
