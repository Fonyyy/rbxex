package com.rbxex.app

/**
 * JNI Bridge to librbxex.so native library
 */
object Bridge {
    init {
        System.loadLibrary("rbxex")
    }

    /** Execute a Lua script. Returns empty string on success, error on failure. */
    external fun executeScript(script: String): String

    /** Returns true if executor has captured a lua_State and is ready */
    external fun isReady(): Boolean

    /** Returns human-readable status string */
    external fun getStatus(): String
}
