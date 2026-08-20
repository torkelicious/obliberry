include(FetchContent)

FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG 18cef5e031d8c6973d80284c67f60549fafd78c1
)

FetchContent_MakeAvailable(imguizmo)

target_link_libraries(imguizmo PUBLIC imgui)
