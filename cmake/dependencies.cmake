include(${CMAKE_CURRENT_LIST_DIR}/dependencies/glfw.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/glad.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/glm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/freetype.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/stb.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/imgui.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/imguizmo.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/nlohmann_json.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/lz4.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/miniaudio.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/nativefiledialog_extended.cmake)

# ObSL Scripting Language
set(OBSL_BUILD_RUNTIME OFF CACHE BOOL "" FORCE)
add_subdirectory(external/obsl)

add_library(obliberry_deps INTERFACE)
target_link_libraries(obliberry_deps INTERFACE
        glfw
        glad
        imgui
        imguizmo
        miniaudio
        stb_image
        freetype
        glm::glm
        nlohmann_json::nlohmann_json
        nfd
        lz4
)
