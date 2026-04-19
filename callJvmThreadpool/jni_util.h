#pragma once
#include <jni.h>
#include <cstdio>

inline bool jni_check(JNIEnv* env, const char* where) {
    if (env->ExceptionCheck()) {
        fprintf(stderr, "JNI exception at %s\n", where);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }
    return true;
}
