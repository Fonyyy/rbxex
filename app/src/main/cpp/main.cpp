#include "luau_hook.h"
#include "bypass.h"
#include "../lua/executor.h"
#include "../utils/memory.h"
#include <jni.h>
#include <string>
#include <android/log.h>
#include <pthread.h>

#define LOG_TAG "RbxEx"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── JNI Interface ─────────────────────────────────────────────────────────

extern "C" {

/**
 * JNI_OnLoad — called the moment our .so is loaded.
 *
 * ORDER IS CRITICAL:
 *   1. Bypass::Initialize() — anti-debug, hide from maps, watchdog
 *   2. Bypass::DelayedInit(5000) schedules hook installation after 5s
 *      This avoids Byfron's boot-time scan window.
 */
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("RbxEx v1.0 — initializing...");

    // ── STEP 1: Run all bypass techniques FIRST ──────────────────────────
    // This must happen before we touch any Roblox memory
    if (!Bypass::Initialize()) {
        LOGE("Bypass initialization failed — proceeding anyway");
    }

    // NOTE: Hooks are installed by Bypass::DelayedInit(5000) after 5 seconds.
    // Do NOT call Hooks::InstallHooks() directly here — that's done by the bypass
    // module on a background thread after the safe window has passed.

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("RbxEx unloading — removing hooks...");
    Hooks::RemoveHooks();
}

// ── Public JNI methods (called from Kotlin Bridge.kt) ─────────────────────

/**
 * Execute a Lua script string.
 * Returns: empty string on success, error message on failure.
 */
JNIEXPORT jstring JNICALL
Java_com_rbxex_app_Bridge_executeScript(JNIEnv* env, jobject thiz, jstring jscript) {
    const char* script_cstr = env->GetStringUTFChars(jscript, nullptr);
    std::string script(script_cstr);
    env->ReleaseStringUTFChars(jscript, script_cstr);

    auto result = Executor::Execute(script);

    if (result.success) {
        return env->NewStringUTF("");
    } else {
        return env->NewStringUTF(result.error.c_str());
    }
}

/**
 * Check if the executor is ready to execute scripts.
 */
JNIEXPORT jboolean JNICALL
Java_com_rbxex_app_Bridge_isReady(JNIEnv* env, jobject thiz) {
    return Executor::IsReady() ? JNI_TRUE : JNI_FALSE;
}

/**
 * Get the current status string.
 */
JNIEXPORT jstring JNICALL
Java_com_rbxex_app_Bridge_getStatus(JNIEnv* env, jobject thiz) {
    std::string status = Executor::GetStatus();
    return env->NewStringUTF(status.c_str());
}

} // extern "C"
