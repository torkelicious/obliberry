include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
)

if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

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
)

target_compile_features(obliberry_tests PRIVATE cxx_std_20)

target_link_libraries(obliberry_tests PRIVATE
    obliberry_engine
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(obliberry_tests)