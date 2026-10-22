include(FetchContent)

FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG 105e4fde45e4e74aae8973d98750820ef7aca7e2
)

FetchContent_MakeAvailable(imguizmo)

target_link_libraries(imguizmo PUBLIC imgui)
