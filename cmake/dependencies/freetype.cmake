include(FetchContent)

FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-14-3
)

set(FT_REQUIRE_ZLIB OFF CACHE BOOL "Disable ZLIB" FORCE)
set(FT_REQUIRE_BZIP2 OFF CACHE BOOL "Disable BZIP2" FORCE)
set(FT_REQUIRE_PNG OFF CACHE BOOL "Disable PNG" FORCE)
set(FT_REQUIRE_HARFBUZZ OFF CACHE BOOL "Disable HarfBuzz" FORCE)
set(FT_REQUIRE_BROTLI OFF CACHE BOOL "Disable Brotli" FORCE)

FetchContent_MakeAvailable(freetype)



