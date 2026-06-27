#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <android/log.h>

#define LOG_TAG "RbxEx"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace Memory {

    // Get base address of a loaded .so module from /proc/self/maps
    uintptr_t GetModuleBase(const char* module_name);

    // Resolve an offset to an absolute address
    inline uintptr_t Resolve(uintptr_t base, uintptr_t offset) {
        return base + offset;
    }

    // Read memory at address safely (returns false on failure)
    template<typename T>
    bool Read(uintptr_t address, T& out) {
        if (address == 0) return false;
        out = *reinterpret_cast<T*>(address);
        return true;
    }

    // Write memory at address
    template<typename T>
    bool Write(uintptr_t address, T value) {
        if (address == 0) return false;
        *reinterpret_cast<T*>(address) = value;
        return true;
    }

    // Pattern scanner — finds a byte pattern in memory range
    // Pattern format: "48 8B ? ? 48 8B" where ? is wildcard
    uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern);

    // Change memory protection to allow read/write/execute
    bool Unprotect(uintptr_t address, size_t size);

} // namespace Memory
