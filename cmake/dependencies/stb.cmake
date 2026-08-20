include(FetchContent)

FetchContent_Declare(
        stb_image
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG 013ac3beddff3dbffafd5177e7972067cd2b5083
)

FetchContent_MakeAvailable(stb_image)

add_library(stb_image STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image/stb_image.cpp
)

target_include_directories(stb_image PUBLIC
        ${stb_image_SOURCE_DIR}
)
