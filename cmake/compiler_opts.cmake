set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Target CPU Architecture Baseline
set(ENGINE_ARCH_LEVEL "x86-64-v2" CACHE STRING "Target CPU architecture (x86-64-v2, x86-64-v3, native)")

if (WIN32)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif ()

# Compiler / Linker Options Target
add_library(obliberry_options INTERFACE)

# Architecture flags (GCC/Clang on x86 only)
if ((CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") AND (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686"))
    target_compile_options(obliberry_options INTERFACE $<$<NOT:$<CONFIG:Debug>>:-march=${ENGINE_ARCH_LEVEL}>)
    message(STATUS "Optimizing for CPU architecture level: ${ENGINE_ARCH_LEVEL}")
endif ()

# Platform-specific compiler & linker options
if (MSVC)
    target_compile_options(obliberry_options INTERFACE
            $<$<NOT:$<CONFIG:Debug>>:/Gy>
            /utf-8 # Force UTF-8 encoding
            /EHsc  # exception handling
    )
    target_link_options(obliberry_options INTERFACE
            $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
            $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>
    )

    # lld-link is generally faster than link.exe,
    # Only takes effect with the Ninja generator;
    # the Visual Studio generator always uses link.exe regardless of CMAKE_LINKER.
    find_program(LLD_LINK_PROGRAM lld-link)
    if (LLD_LINK_PROGRAM AND CMAKE_GENERATOR MATCHES "Ninja")
        set(CMAKE_LINKER ${LLD_LINK_PROGRAM} CACHE FILEPATH "" FORCE)
        message(STATUS "Using lld-link for faster MSVC linking")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(obliberry_options INTERFACE
            $<$<NOT:$<CONFIG:Debug>>:-ffunction-sections>
            $<$<NOT:$<CONFIG:Debug>>:-fdata-sections>
            $<$<CONFIG:RelWithDebInfo>:-fno-omit-frame-pointer>
            $<$<CONFIG:RelWithDebInfo>:-g>
    )

    if (APPLE)
        # macOS specific linker flags
        target_link_options(obliberry_options INTERFACE $<$<NOT:$<CONFIG:Debug>>:-Wl,-dead_strip>)
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        # Linux specific linker flags
        target_link_options(obliberry_options INTERFACE
                $<$<AND:$<NOT:$<CONFIG:Debug>>,$<NOT:$<BOOL:${ENABLE_SANITIZERS}>>>:-Wl,--gc-sections>
                $<$<CONFIG:RelWithDebInfo>:-Wl,--build-id>
                $<$<AND:$<CONFIG:Release,MinSizeRel>,$<NOT:$<BOOL:${ENABLE_SANITIZERS}>>>:-s>
        )
    endif ()
endif ()

# Alternative linkers for Linux/GCC
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_program(MOLD_LINKER mold)
    find_program(LLD_LINKER lld)
    if (MOLD_LINKER)
        target_link_options(obliberry_options INTERFACE "-fuse-ld=mold")
        message(STATUS "Using MOLD linker")
    elseif (LLD_LINKER)
        target_link_options(obliberry_options INTERFACE "-fuse-ld=lld")
        message(STATUS "Using LLD linker")
    endif ()
endif ()

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
endif ()

# Ccache configuration
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM AND NOT CMAKE_C_COMPILER_LAUNCHER AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    # only fall back to ccache when nothing was already requested
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "ccache found and enabled")
endif ()

# LTO
# Skipped entirely when sanitizers are enabled
if (ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled. skipping LTO/IPO")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang gets ThinLTO
    target_compile_options(obliberry_options INTERFACE
            $<$<CONFIG:Release,MinSizeRel,RelWithDebInfo>:-flto=thin>
    )
    target_link_options(obliberry_options INTERFACE
            $<$<CONFIG:Release,MinSizeRel,RelWithDebInfo>:-flto=thin>
    )
    message(STATUS "ThinLTO enabled for Clang Release/RelWithDebInfo/MinSizeRel builds.")
else ()
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported OUTPUT error)
    if (lto_supported)
        message(STATUS "LTO/IPO enabled for Release and RelWithDebInfo builds.")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    else ()
        message(WARNING "LTO/IPO is not supported by your compiler: ${error}")
    endif ()
endif ()

if (ENABLE_SANITIZERS)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizers are only supported with GCC or Clang")
    endif ()
    target_compile_options(obliberry_options INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -g
    )
    target_link_options(obliberry_options INTERFACE
            -fsanitize=address,undefined
    )
    message(STATUS "Sanitizers enabled")
endif ()
