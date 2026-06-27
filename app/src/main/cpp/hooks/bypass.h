#pragma once
#include <cstdint>
#include <cstddef>

namespace Bypass {

    // ── Main entry point — run ALL bypass techniques ─────────────────────
    // Call this FIRST in JNI_OnLoad, before installing any hooks
    bool Initialize();

    // ── Individual techniques ─────────────────────────────────────────────

    // 1. Hide our .so from /proc/self/maps (linker unlinking)
    bool HideFromMaps(const char* lib_name);

    // 2. Patch TracerPid check (anti-debug bypass)
    bool PatchAntiDebug();

    // 3. Randomize our .so name in memory to avoid string scans
    bool ObfuscateLibraryName();

    // 4. Spoof /proc/self/status reads (hide PID/TracerPid)
    bool HookProcStatus();

    // 5. Patch Byfron's integrity checker (restores hooks after Byfron resets them)
    bool WatchdogThread();

    // 6. Delayed execution — wait before hooking to avoid boot-time scan window
    void DelayedInit(int delay_ms);

} // namespace Bypass
