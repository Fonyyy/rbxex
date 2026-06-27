#include "memory.h"
#include <fstream>
#include <sstream>
#include <sys/mman.h>
#include <dlfcn.h>
#include <cstring>
#include <vector>
#include <android/log.h>

namespace Memory {

uintptr_t GetModuleBase(const char* module_name) {
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        LOGE("Failed to open /proc/self/maps");
        return 0;
    }
    std::string line;
    while (std::getline(maps, line)) {
        if (line.find(module_name) == std::string::npos) continue;
        // Format: "startaddr-endaddr perms offset dev inode pathname"
        uintptr_t start = 0;
        sscanf(line.c_str(), "%lx-", &start);
        LOGI("Module '%s' base: 0x%lX", module_name, start);
        return start;
    }
    LOGE("Module '%s' not found in /proc/self/maps", module_name);
    return 0;
}

bool Unprotect(uintptr_t address, size_t size) {
    uintptr_t page_start = address & ~(getpagesize() - 1);
    int ret = mprotect(reinterpret_cast<void*>(page_start), size + (address - page_start),
                       PROT_READ | PROT_WRITE | PROT_EXEC);
    if (ret != 0) {
        LOGE("mprotect failed for 0x%lX", address);
        return false;
    }
    return true;
}

// Parse pattern string like "48 8B ? ? 48" into bytes/mask
static bool ParsePattern(const char* pattern, std::vector<uint8_t>& bytes,
                         std::vector<bool>& mask) {
    const char* p = pattern;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        if (*p == '?') {
            bytes.push_back(0x00);
            mask.push_back(false); // wildcard
            p++;
            if (*p == '?') p++;
        } else {
            char hex[3] = {p[0], p[1], 0};
            bytes.push_back((uint8_t)strtol(hex, nullptr, 16));
            mask.push_back(true); // must match
            p += 2;
        }
    }
    return !bytes.empty();
}

uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern) {
    std::vector<uint8_t> bytes;
    std::vector<bool> mask;
    if (!ParsePattern(pattern, bytes, mask)) return 0;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(start);
    size_t plen = bytes.size();

    for (size_t i = 0; i + plen <= size; i++) {
        bool found = true;
        for (size_t j = 0; j < plen; j++) {
            if (mask[j] && data[i + j] != bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            LOGI("Pattern found at 0x%lX", start + i);
            return start + i;
        }
    }
    return 0;
}

} // namespace Memory
