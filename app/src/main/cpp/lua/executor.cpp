#include "executor.h"
#include "../utils/memory.h"
#include "../utils/offsets.h"
#include "lua_types.h"
#include <android/log.h>
#include <string>
#include <mutex>

#define LOG_TAG "RbxEx-Executor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace Executor {

static uintptr_t g_roblox_base = 0;
static lua_State* g_main_state = nullptr;
static std::mutex g_exec_mutex;

// Resolved function pointers
static lua_newthread_t    r_lua_newthread    = nullptr;
static lua_pcall_t        r_lua_pcall        = nullptr;
static lua_pushstring_t   r_lua_pushstring   = nullptr;
static lua_pushcclosure_t r_lua_pushcclosure = nullptr;
static lua_tostring_t     r_lua_tostring     = nullptr;
static lua_settop_t       r_lua_settop       = nullptr;
static lua_gettop_t       r_lua_gettop       = nullptr;
static luaL_loadbuffer_t  r_luaL_loadbuffer  = nullptr;
static SetThreadIdentity_t r_SetThreadIdentity = nullptr;

// ── Resolve function pointers from base + offset ──────────────────────────
static bool ResolveAll() {
    if (g_roblox_base == 0) {
        g_roblox_base = Memory::GetModuleBase("libroblox.so");
        if (g_roblox_base == 0) {
            LOGE("Failed to find libroblox.so base!");
            return false;
        }
        LOGI("libroblox.so base: 0x%lX", g_roblox_base);
    }

    #define RESOLVE(fn, offset) \
        r_##fn = (fn##_t)(g_roblox_base + offset); \
        LOGI(#fn " resolved to: 0x%lX", (uintptr_t)r_##fn);

    // Only resolve offsets that are non-zero
    if (Offsets::lua_newthread)    RESOLVE(lua_newthread,    Offsets::lua_newthread)
    if (Offsets::lua_pcall)        RESOLVE(lua_pcall,        Offsets::lua_pcall)
    if (Offsets::lua_pushstring)   RESOLVE(lua_pushstring,   Offsets::lua_pushstring)
    if (Offsets::lua_pushcclosure) RESOLVE(lua_pushcclosure, Offsets::lua_pushcclosure)
    if (Offsets::lua_tostring)     RESOLVE(lua_tostring,     Offsets::lua_tostring)
    if (Offsets::lua_settop)       r_lua_settop = (lua_settop_t)(g_roblox_base + Offsets::lua_settop);
    if (Offsets::lua_gettop)       r_lua_gettop = (lua_gettop_t)(g_roblox_base + Offsets::lua_gettop);
    if (Offsets::luaL_loadbuffer)  RESOLVE(luaL_loadbuffer,  Offsets::luaL_loadbuffer)
    if (Offsets::SetThreadIdentity) r_SetThreadIdentity = (SetThreadIdentity_t)(g_roblox_base + Offsets::SetThreadIdentity);

    #undef RESOLVE
    return true;
}

// ── Called by hook when we capture a valid lua_State ──────────────────────
void CaptureState(lua_State* L) {
    if (g_main_state == nullptr && L != nullptr) {
        g_main_state = L;
        LOGI("Captured lua_State: %p", (void*)L);
        ResolveAll();
    }
}

// ── Execute a Lua script string ───────────────────────────────────────────
ExecuteResult Execute(const std::string& script) {
    std::lock_guard<std::mutex> lock(g_exec_mutex);

    if (g_main_state == nullptr) {
        return {false, "No lua_State captured yet. Make sure Roblox is running."};
    }

    if (!r_lua_newthread || !r_lua_pcall || !r_luaL_loadbuffer) {
        return {false, "Function pointers not resolved. Offsets need to be updated."};
    }

    // Create a new thread (coroutine) to isolate our execution
    lua_State* L = r_lua_newthread(g_main_state);
    if (!L) {
        return {false, "Failed to create new Lua thread"};
    }

    // Elevate thread identity to level 8 (maximum — same as CoreScripts)
    if (r_SetThreadIdentity) {
        r_SetThreadIdentity(L, 8);
        LOGI("Thread identity set to 8");
    } else {
        LOGE("SetThreadIdentity not resolved — running at default identity!");
    }

    // Load the script as a Lua chunk
    int load_result = r_luaL_loadbuffer(L, script.c_str(), script.size(), "RbxEx");
    if (load_result != 0) {
        const char* err = r_lua_tostring ? r_lua_tostring(L, -1) : "unknown error";
        std::string error_msg = err ? err : "Failed to load script";
        LOGE("Script load error: %s", error_msg.c_str());
        return {false, "Load error: " + error_msg};
    }

    // Execute the loaded chunk (protected call, 0 args, no return values)
    int call_result = r_lua_pcall(L, 0, 0, 0);
    if (call_result != 0) {
        const char* err = r_lua_tostring ? r_lua_tostring(L, -1) : "unknown error";
        std::string error_msg = err ? err : "Runtime error";
        LOGE("Script execution error: %s", error_msg.c_str());
        return {false, "Runtime error: " + error_msg};
    }

    LOGI("Script executed successfully");
    return {true, ""};
}

bool IsReady() {
    return g_main_state != nullptr;
}

std::string GetStatus() {
    if (g_roblox_base == 0) return "Not attached";
    if (g_main_state == nullptr) return "Waiting for Roblox to load...";
    return "Ready";
}

} // namespace Executor
