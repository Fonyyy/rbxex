#pragma once
#include <cstdint>
#include <cstddef>

/**
 * RbxEx Offsets — Roblox Android 2.727.1199 arm64-v8a
 * Base address is determined at runtime via /proc/self/maps
 *
 * HOW TO UPDATE:
 * 1. Run analyze.py on new libroblox.so to get string VAs
 * 2. Use Ghidra to find XREF code addresses from those strings
 * 3. Update the values below
 */

namespace Offsets {

    // ── Luau Interpreter ─────────────────────────────────────────
    // luau_execute: Found via Ghidra Functions sorted by size (311432 bytes = largest)
    constexpr uintptr_t luau_execute       = 0x02BD2460;

    // lua_newstate: Creates a new Lua state — find via string "not enough memory"
    constexpr uintptr_t lua_newstate       = 0x0; // TODO: find

    // lua_close: Closes a Lua state
    constexpr uintptr_t lua_close          = 0x0; // TODO: find

    // lua_newthread: Creates a new Lua coroutine/thread
    constexpr uintptr_t lua_newthread      = 0x0; // TODO: find

    // ── Script Execution ──────────────────────────────────────────
    // luau_load: Loads a Luau bytecode chunk
    constexpr uintptr_t luau_load          = 0x0; // TODO: find

    // lua_pcall: Protected call
    constexpr uintptr_t lua_pcall          = 0x0; // TODO: find

    // lua_resume: Resume a coroutine
    constexpr uintptr_t lua_resume         = 0x0; // TODO: find

    // ── Stack Operations ──────────────────────────────────────────
    constexpr uintptr_t lua_pushstring     = 0x0; // TODO: find
    constexpr uintptr_t lua_pushnumber     = 0x0; // TODO: find
    constexpr uintptr_t lua_pushboolean    = 0x0; // TODO: find
    constexpr uintptr_t lua_pushnil        = 0x0; // TODO: find
    constexpr uintptr_t lua_pushcclosure   = 0x0; // TODO: find
    constexpr uintptr_t lua_pushvalue      = 0x0; // TODO: find

    constexpr uintptr_t lua_tostring       = 0x0; // TODO: find
    constexpr uintptr_t lua_tonumber       = 0x0; // TODO: find
    constexpr uintptr_t lua_toboolean      = 0x0; // TODO: find
    constexpr uintptr_t lua_type           = 0x0; // TODO: find

    // ── Table Operations ──────────────────────────────────────────
    constexpr uintptr_t lua_getfield       = 0x0; // TODO: find
    constexpr uintptr_t lua_setfield       = 0x0; // TODO: find
    constexpr uintptr_t lua_rawget         = 0x0; // TODO: find
    constexpr uintptr_t lua_rawset         = 0x0; // TODO: find
    constexpr uintptr_t lua_newtable       = 0x0; // TODO: find
    constexpr uintptr_t lua_getglobal      = 0x0; // TODO: find
    constexpr uintptr_t lua_setglobal      = 0x0; // TODO: find

    // ── Auxiliary Library ─────────────────────────────────────────
    constexpr uintptr_t luaL_loadstring    = 0x0; // TODO: find
    constexpr uintptr_t luaL_loadbuffer    = 0x0; // TODO: find
    constexpr uintptr_t luaL_newstate      = 0x0; // TODO: find
    constexpr uintptr_t luaL_openlibs      = 0x0; // TODO: find
    constexpr uintptr_t luaL_error         = 0x0; // TODO: find

    // ── Roblox-specific ───────────────────────────────────────────
    // ScriptContext::executeInNewThread — runs a script on the scheduler
    constexpr uintptr_t ScriptContext_executeInNewThread = 0x0; // TODO: find

    // Identity setter — sets thread identity level (0-8, we need 8)
    constexpr uintptr_t SetThreadIdentity = 0x0; // TODO: find
    constexpr uintptr_t GetThreadIdentity = 0x0; // TODO: find

    // TaskScheduler::singleton — global instance
    constexpr uintptr_t TaskScheduler_singleton = 0x0; // TODO: find

} // namespace Offsets

// ── Luau lua_State structure offsets ──────────────────────────────────────
// These are the byte offsets WITHIN the lua_State struct
// Standard Lua 5.1 / Luau layout (Roblox may differ slightly)
namespace StateOffsets {
    constexpr size_t top          = 0x08;  // StkId top — stack pointer
    constexpr size_t base         = 0x10;  // StkId base
    constexpr size_t global       = 0x18;  // global_State* l_G
    constexpr size_t ci           = 0x20;  // CallInfo* ci
    constexpr size_t identity     = 0x30;  // int identity (Roblox-specific field)
    constexpr size_t userdata     = 0x40;  // userdata pointer (Roblox-specific)
}
