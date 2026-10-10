include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
)

if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(obliberry_tests
        ColliderTests.cpp
        CollisionMovementTests.cpp
        RegistryTests.cpp
        EntityUUIDTests.cpp
        EntitySerializationTests.cpp
        SaveGameTests.cpp
        HierarchyTests.cpp
        MapTests.cpp
        AnimationTests.cpp
        SceneTests.cpp
        ObSLTests.cpp
        ObSLBindingTests.cpp
        AssetTests.cpp
        ScriptAssetAnalyzerTests.cpp
        ExportDependencyTests.cpp
        PrefabAssetScopeTests.cpp
        VFSPackageTests.cpp
        ConfigurationTests.cpp
        EditorCommandTests.cpp
        AudioIntegrationTests.cpp
        ../src/Applications/Editor/Commands/UndoManager.cpp
        ../src/Applications/Editor/Commands/EditorCommands.cpp
        ../src/Applications/Editor/Commands/CustomDataSnapshot.cpp
        ../src/IO/Package/Tools/ObpakTools.cpp
)

target_compile_features(obliberry_tests PRIVATE cxx_std_20)

target_link_libraries(obliberry_tests PRIVATE
        obliberry_engine
        GTest::gtest_main
)

option(OBLIBERRY_TEST_RENDERING "Run integration tests that require OpenGL 4.3" OFF)
if (OBLIBERRY_TEST_RENDERING)
    target_sources(obliberry_tests PRIVATE RenderingIntegrationTests.cpp)
endif ()

include(GoogleTest)
gtest_discover_tests(obliberry_tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30)
