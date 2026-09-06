include(FetchContent)

FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG 0.11.25
        GIT_SHALLOW TRUE
)

# configuration
set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)

# audio formats
set(MINIAUDIO_NO_LIBVORBIS ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBOPUS   ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_FLAC      ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(miniaudio)

