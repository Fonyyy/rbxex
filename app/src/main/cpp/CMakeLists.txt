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

# ── Dobby hooking library ─────────────────────────────────────────────────
# Dobby is an inline hooking library for ARM64
# Download prebuilt from: https://github.com/jmpews/Dobby/releases
# Place libdobby.a in: app/src/main/cpp/libs/arm64-v8a/
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/arm64-v8a/libdobby.a
)

# ── Our executor library ──────────────────────────────────────────────────
add_library(
    rbxex                          # Output: librbxex.so
    SHARED

    # Entry point
    main.cpp

    # Hooks
    hooks/luau_hook.cpp

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
)

# Link libraries
target_link_libraries(
    rbxex
    dobby           # Inline hooking
    android         # Android APIs
    log             # __android_log_print
    dl              # dlopen/dlsym
)
