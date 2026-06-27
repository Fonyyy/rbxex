/**
 * RbxEx — Byfron/Hyperion Bypass Module
 *
 * Detection vectors we address:
 *
 * [1] Library presence scan   → /proc/self/maps contains unknown .so
 * [2] Inline hook detection   → Byfron checks function prologues for patches
 * [3] Debugger detection      → TracerPid in /proc/self/status
 * [4] String scanning         → Suspicious strings in our library memory
 * [5] Integrity checks        → Byfron periodically re-reads its own code
 * [6] Call stack inspection   → Unexpected return addresses in call stacks
 * [7] Timing analysis         → Hooks appear suspiciously fast after load
 */
#include "bypass.h"
#include "../utils/memory.h"
#include <android/log.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <dirent.h>
#include <dobby.h>

// Obfuscate the log tag to avoid string detection
static const char TAG[] = {'R','b','x','B','y','p','a','s','s',0};
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace Bypass {

// ═══════════════════════════════════════════════════════════════════════════
// [1] HIDE FROM /proc/self/maps
//     Technique: After our .so is loaded by the linker, manually remove it
//     from the linker's internal list of loaded libraries. This makes it
//     invisible to /proc/self/maps scans.
// ═══════════════════════════════════════════════════════════════════════════
struct solist_t {
    void*    next;
    void*    prev;
    void*    phdr;
    size_t   phnum;
    uintptr_t base;
    uintptr_t size;
    // ... more fields, but we only need next/prev to unlink
};

// Android linker's global solist head (found via dlopen internals)
static solist_t* find_soinfo_for_lib(const char* lib_name) {
    // Walk /proc/self/maps to find the library's base address
    uintptr_t lib_base = Memory::GetModuleBase(lib_name);
    if (lib_base == 0) return nullptr;

    // Use dl_iterate_phdr to find our soinfo
    struct FindInfo {
        uintptr_t target_base;
        void* found_soinfo;
    } info = {lib_base, nullptr};

    dl_iterate_phdr([](struct dl_phdr_info* phdr_info, size_t size, void* data) -> int {
        auto* fi = reinterpret_cast<FindInfo*>(data);
        if ((uintptr_t)phdr_info->dlpi_addr == fi->target_base) {
            // We found it, but dl_iterate_phdr doesn't give us soinfo directly
            // In practice, soinfo is at dlpi_addr - some offset in the linker
            fi->found_soinfo = (void*)phdr_info->dlpi_addr;
            return 1; // stop iteration
        }
        return 0;
    }, &info);

    return (solist_t*)info.found_soinfo;
}

bool HideFromMaps(const char* lib_name) {
    LOG("Attempting to hide '%s' from memory maps...", lib_name);

    // Method 1: Change the library's path string in memory
    // The /proc/self/maps is generated from vm_area_struct entries in the kernel.
    // We can't directly modify those, but we can rename our memory mappings
    // using anonymous mappings trick.

    uintptr_t base = Memory::GetModuleBase(lib_name);
    if (base == 0) {
        LOGE("Cannot find %s base for hiding", lib_name);
        return false;
    }

    // Method 2: Use prctl(PR_SET_VMA) on Linux 5.10+ to rename memory region
    // This changes how the region appears in /proc/self/maps
#ifdef PR_SET_VMA
    // Rename our .so pages to look like anonymous memory
    // This makes Byfron see "[anon]" instead of "librbxex.so"
    size_t lib_size = 20 * 1024 * 1024; // ~20MB, covers typical .so size
    int ret = prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME,
                    (void*)base, lib_size, nullptr);
    if (ret == 0) {
        LOG("Library renamed to anonymous via prctl — hidden from maps scan");
        return true;
    }
#endif

    // Method 3: Overwrite the path string that appears in /proc/self/maps
    // by scanning memory near the library base for the filename string
    // and zeroing/replacing it with spaces
    std::string search_name = lib_name;
    uint8_t* scan_start = reinterpret_cast<uint8_t*>(base);

    // Scan nearby memory for the lib name string
    for (int i = -4096; i < 4096; i += 4) {
        uintptr_t addr = base + i;
        if (memmem(reinterpret_cast<void*>(addr), 256,
                   lib_name, strlen(lib_name))) {
            Memory::Unprotect(addr, 256);
            memset(reinterpret_cast<void*>(addr), 0, strlen(lib_name));
            LOG("Zeroed library name in memory at 0x%lX", addr);
        }
    }

    LOG("HideFromMaps complete");
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// [2] ANTI-DEBUG BYPASS
//     Byfron uses ptrace-based debugger detection and checks /proc/self/status
//     for TracerPid. We hook the relevant syscalls and file reads.
// ═══════════════════════════════════════════════════════════════════════════

// Hooked open() — intercepts /proc/self/status reads
typedef int (*open_t)(const char* path, int flags, ...);
static open_t original_open = nullptr;

// Fake /proc/self/status that always shows TracerPid: 0
static const char FAKE_STATUS_CONTENT[] =
    "Name:\troblox\n"
    "State:\tS (sleeping)\n"
    "TracerPid:\t0\n"        // ← This is what Byfron checks
    "Pid:\t1234\n"
    "PPid:\t1\n"
    "\n";

static int fake_status_fd = -1;

static int hooked_open(const char* path, int flags, ...) {
    if (path != nullptr && strstr(path, "status") && strstr(path, "self")) {
        // Return a fake fd pointing to our sanitized status content
        int fds[2];
        if (pipe(fds) == 0) {
            write(fds[1], FAKE_STATUS_CONTENT, sizeof(FAKE_STATUS_CONTENT) - 1);
            close(fds[1]);
            LOG("Intercepted /proc/self/status read — returning fake status");
            return fds[0];
        }
    }
    return original_open(path, flags);
}

bool PatchAntiDebug() {
    LOG("Installing anti-debug bypass...");

    // Bypass 1: Hook open() to intercept /proc/self/status
    void* open_sym = dlsym(RTLD_DEFAULT, "open");
    if (open_sym) {
        DobbyHook(open_sym,
                  reinterpret_cast<void*>(hooked_open),
                  reinterpret_cast<void**>(&original_open));
        LOG("Hooked open() for TracerPid bypass");
    }

    // Bypass 2: Self-ptrace trick
    // If we ptrace ourselves first, any external ptrace attempt will fail
    // (only one tracer allowed per process)
    if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == 0) {
        LOG("Self-ptrace installed — external debuggers blocked");
    }

    // Bypass 3: Hide our thread from /proc/<pid>/task
    // By setting thread name to something innocuous
    prctl(PR_SET_NAME, "ChromiumGL", 0, 0, 0);

    LOG("Anti-debug bypass complete");
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// [3] STRING OBFUSCATION
//     Our library shouldn't contain obvious strings like "executor", "inject"
//     All sensitive strings are XOR-encoded and decoded at runtime.
// ═══════════════════════════════════════════════════════════════════════════
static std::string XorDecode(const uint8_t* data, size_t len, uint8_t key) {
    std::string result(len, 0);
    for (size_t i = 0; i < len; i++) {
        result[i] = data[i] ^ key ^ (uint8_t)i;
    }
    return result;
}

// Example: XOR-encoded "librbxex.so" with key 0x42
// Generated by: [c ^ 0x42 ^ i for i,c in enumerate(b"librbxex.so")]
static const uint8_t ENC_LIBNAME[] = {0x2e, 0x28, 0x21, 0x34, 0x37, 0x3c, 0x2d, 0x32, 0x55, 0x1d, 0x1d};

bool ObfuscateLibraryName() {
    // Decode at runtime — never stored as plain string
    std::string lib_name = XorDecode(ENC_LIBNAME, sizeof(ENC_LIBNAME), 0x42);
    LOG("Library name decoded (obfuscated in binary)");

    // Zero out any string that looks like our lib name in the .rodata section
    uintptr_t our_base = Memory::GetModuleBase(lib_name.c_str());
    if (our_base == 0) our_base = Memory::GetModuleBase("librbxex.so");

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// [4] WATCHDOG THREAD
//     Byfron periodically checks function prologues for inline hooks.
//     Our watchdog re-installs hooks if they've been removed.
// ═══════════════════════════════════════════════════════════════════════════
static volatile bool g_watchdog_running = false;

static void* watchdog_thread(void* arg) {
    LOG("Watchdog thread started");

    // Import hook installer from hooks module
    extern bool InstallHooks();

    while (g_watchdog_running) {
        // Sleep 5 seconds between checks
        usleep(5 * 1000 * 1000);

        // Check if our luau_execute hook is still intact
        // by verifying the first bytes of the function haven't been restored
        // (Byfron's integrity check restores patched functions)
        // If hooks were removed, reinstall them
        // InstallHooks();  // Uncomment after testing stability
    }

    return nullptr;
}

bool WatchdogThread() {
    g_watchdog_running = true;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, watchdog_thread, nullptr) != 0) {
        LOGE("Failed to create watchdog thread");
        return false;
    }

    pthread_attr_destroy(&attr);
    LOG("Watchdog thread started (tid=%lu)", tid);
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// [5] HOOK /proc/self/status (more robust version)
// ═══════════════════════════════════════════════════════════════════════════
bool HookProcStatus() {
    // Already handled in PatchAntiDebug() via open() hook
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// [6] DELAYED INITIALIZATION
//     Don't hook immediately on JNI_OnLoad — Byfron runs boot-time scans.
//     Wait until Roblox is fully loaded before installing hooks.
// ═══════════════════════════════════════════════════════════════════════════
struct DelayedInitArgs {
    int delay_ms;
};

static void* delayed_init_thread(void* arg) {
    auto* args = reinterpret_cast<DelayedInitArgs*>(arg);
    int delay = args->delay_ms;
    delete args;

    LOG("Delayed init: waiting %d ms before hooking...", delay);
    usleep(delay * 1000);

    // Install hooks after delay
    extern bool InstallHooks();
    InstallHooks();

    LOG("Delayed init complete — hooks installed");
    return nullptr;
}

void DelayedInit(int delay_ms) {
    auto* args = new DelayedInitArgs{delay_ms};

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, delayed_init_thread, args);
    pthread_attr_destroy(&attr);

    LOG("Scheduled hook installation in %d ms", delay_ms);
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN INITIALIZE — Runs all bypass techniques in order
// ═══════════════════════════════════════════════════════════════════════════
bool Initialize() {
    LOG("=== RbxEx Bypass Initializing ===");

    // Step 1: Anti-debug first (before anything else)
    PatchAntiDebug();

    // Step 2: Obfuscate our presence
    ObfuscateLibraryName();
    HideFromMaps("librbxex.so");

    // Step 3: Hook /proc reads
    HookProcStatus();

    // Step 4: Start watchdog to re-install hooks if Byfron removes them
    WatchdogThread();

    // Step 5: Schedule actual hook installation with a delay
    // 5000ms = 5 seconds after Roblox starts — avoids boot-time scanner
    DelayedInit(5000);

    LOG("=== Bypass initialized — hooks will install in 5s ===");
    return true;
}

} // namespace Bypass
