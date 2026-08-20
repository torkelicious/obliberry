include(FetchContent)

set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        lz4
        GIT_REPOSITORY https://github.com/lz4/lz4.git
        GIT_TAG v1.10.0
        SOURCE_SUBDIR build/cmake
)

FetchContent_MakeAvailable(lz4)

# alias for LZ4 target if missing
if (TARGET lz4_static AND NOT TARGET lz4)
    add_library(lz4 ALIAS lz4_static)
endif ()
