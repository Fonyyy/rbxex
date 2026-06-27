#include "luau_hook.h"
#include "../lua/executor.h"
#include "../utils/memory.h"
#include <jni.h>
#include <string>
#include <android/log.h>
#include <pthread.h>

#define LOG_TAG "RbxEx-Main"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── JNI Interface ─────────────────────────────────────────────────────────
// These functions are called from Kotlin via JNI

extern "C" {

/**
 * Called when our .so is loaded by Roblox (via patched AndroidManifest or JVMTI).
 * This is the entry point — install hooks here.
 */
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("RbxEx v1.0 loaded!");
    LOGI("JNI_OnLoad called — installing hooks...");

    // Install luau_execute hook
    if (!Hooks::InstallHooks()) {
        LOGE("Failed to install hooks! Executor will not work.");
    }

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
