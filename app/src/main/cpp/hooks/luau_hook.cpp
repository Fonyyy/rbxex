#include "luau_hook.h"
#include "../lua/executor.h"
#include "../utils/memory.h"
#include "../utils/offsets.h"
#include "../lua/lua_types.h"
#include <android/log.h>
#include <dobby.h>

#define LOG_TAG "RbxEx-Hook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace Hooks {

static uintptr_t g_base = 0;
static bool g_hooked = false;
static int g_state_capture_count = 0;

// ── Original luau_execute function pointer ────────────────────────────────
// Signature from Luau source: void luau_execute(lua_State* L)
typedef void (*luau_execute_t)(lua_State* L);
static luau_execute_t original_luau_execute = nullptr;

// ── Our hook for luau_execute ─────────────────────────────────────────────
// Called every time Roblox executes any Lua code
static void hook_luau_execute(lua_State* L) {
    // Capture the state on first few calls (to ensure it's a "real" game state)
    if (g_state_capture_count < 5 && L != nullptr) {
        g_state_capture_count++;
        LOGI("hook_luau_execute called, L=%p (capture #%d)", (void*)L, g_state_capture_count);

        // After a few calls, pick a stable state
        if (g_state_capture_count == 3) {
            Executor::CaptureState(L);
        }
    }

    // Always call the original to not break Roblox
    if (original_luau_execute) {
        original_luau_execute(L);
    }
}

// ── Install all hooks ─────────────────────────────────────────────────────
bool InstallHooks() {
    if (g_hooked) {
        LOGI("Hooks already installed");
        return true;
    }

    g_base = Memory::GetModuleBase("libroblox.so");
    if (g_base == 0) {
        LOGE("Cannot find libroblox.so base — are we inside Roblox?");
        return false;
    }

    LOGI("libroblox.so base: 0x%lX", g_base);

    // ── Hook luau_execute ──────────────────────────────────────────────
    if (Offsets::luau_execute != 0) {
        uintptr_t target = g_base + Offsets::luau_execute;
        LOGI("Hooking luau_execute @ 0x%lX (offset 0x%lX)", target, Offsets::luau_execute);

        // Make memory writable (needed for inline hooking)
        Memory::Unprotect(target, 16);

        // Install hook with Dobby
        DobbyHook(reinterpret_cast<void*>(target),
                  reinterpret_cast<void*>(hook_luau_execute),
                  reinterpret_cast<void**>(&original_luau_execute));

        LOGI("luau_execute hooked successfully");
    } else {
        LOGE("luau_execute offset is 0 — hook not installed! Update offsets.h");
        // Still return true to allow the app to load; Execute() will report "not ready"
    }

    g_hooked = true;
    return true;
}

// ── Remove all hooks (cleanup) ────────────────────────────────────────────
void RemoveHooks() {
    if (!g_hooked) return;

    if (Offsets::luau_execute != 0 && original_luau_execute) {
        uintptr_t target = g_base + Offsets::luau_execute;
        DobbyDestroy(reinterpret_cast<void*>(target));
        LOGI("luau_execute hook removed");
    }

    g_hooked = false;
}

} // namespace Hooks
