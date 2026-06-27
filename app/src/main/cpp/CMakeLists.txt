cmake_minimum_required(VERSION 3.22.1)
project("rbxex")

# ── Build settings ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Strip symbols in release to reduce size and hinder RE
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fvisibility=hidden -fstack-protector-strong")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--strip-all")
endif()

# ── Dobby hooking library (Build from source) ─────────────────────────────
include(FetchContent)
FetchContent_Declare(
    dobby
    GIT_REPOSITORY https://github.com/jmpews/Dobby.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(dobby)

# ── Our executor library ──────────────────────────────────────────────────
add_library(
    rbxex                          # Output: librbxex.so
    SHARED

    # Entry point
    main.cpp

    # Hooks & Bypass
    hooks/luau_hook.cpp
    hooks/bypass.cpp

    # Lua executor
    lua/executor.cpp

    # Utilities
    utils/memory.cpp
)

# Include directories
target_include_directories(rbxex PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/lua
    ${CMAKE_SOURCE_DIR}/hooks
    ${dobby_SOURCE_DIR}/include   # Dobby headers from source
)

# Link libraries
target_link_libraries(
    rbxex
    dobby           # Inline hooking
    android         # Android APIs
    log             # __android_log_print
    dl              # dlopen/dlsym
)
