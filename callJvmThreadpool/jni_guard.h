#pragma once
#include <jni.h>

// RAII guard for JavaVM lifetime.
// Calls DestroyJavaVM on destruction.
struct JvmGuard {
    JavaVM* vm = nullptr;
    ~JvmGuard() {
        if (vm) vm->DestroyJavaVM();
    }
    JvmGuard() = default;
    JvmGuard(const JvmGuard&) = delete;
    JvmGuard& operator=(const JvmGuard&) = delete;
};

// RAII guard for JNI thread attachment.
// Calls DetachCurrentThread on destruction if attached.
struct JniEnvGuard {
    JavaVM* vm = nullptr;
    bool attached = false;
    ~JniEnvGuard() {
        if (vm && attached) vm->DetachCurrentThread();
    }
    JniEnvGuard() = default;
    JniEnvGuard(const JniEnvGuard&) = delete;
    JniEnvGuard& operator=(const JniEnvGuard&) = delete;
};
