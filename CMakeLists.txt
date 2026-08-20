cmake_minimum_required(VERSION 3.23)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(obliberry LANGUAGES CXX C)

option(ENABLE_UNITY_BUILD "Enable unity builds" OFF)
option(BUILD_PACK_TOOLS "Build .obpak packager tools" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CONFIG_TYPES Debug Release RelWithDebInfo MinSizeRel)

include(cmake/compiler_opts.cmake)
include(cmake/utils.cmake)
include(cmake/dependencies.cmake)

add_subdirectory(src)
