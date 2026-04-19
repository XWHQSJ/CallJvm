#include <iostream>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include "jni_util.h"

static const char* get_classpath() {
    const char* cp = getenv("CALLJVM_CLASSPATH");
    return cp ? cp : ".";
}

int main()
{
    JavaVMOption options[3];
    JNIEnv *env;
    JavaVM *jvm;
    JavaVMInitArgs vm_args;
    long status;
    jclass cls;
    jmethodID mid;
    jobject jobj;

    std::string cp_opt = std::string("-Djava.class.path=") + get_classpath();

    options[0].optionString = const_cast<char *>("-Djava.compiler=NONE");
    options[1].optionString = const_cast<char *>(cp_opt.c_str());
    options[2].optionString = const_cast<char *>("-verbose:jni");
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 3;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    status = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);

    if (status != JNI_ERR) {
        printf("create java jvm success\n");

        // Try SqlEcho first (new example), fall back to Helloworld (legacy)
        cls = env->FindClass("com/xwhqsj/example/SqlEcho");
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            printf("SqlEcho not found, trying legacy Helloworld...\n");
            cls = env->FindClass("com/testjvm/Helloworld");
        }

        if (!jni_check(env, "FindClass")) {
            jvm->DestroyJavaVM();
            return -1;
        }

        if (cls != nullptr) {
            printf("find java class success\n");

            // Check if this is SqlEcho (has static process method)
            mid = env->GetStaticMethodID(cls, "process",
                "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
            if (!env->ExceptionCheck() && mid != nullptr) {
                // SqlEcho path: call process("SELECT * FROM users", "demo")
                jstring jsql = env->NewStringUTF("SELECT * FROM users");
                jstring jdb = env->NewStringUTF("demo");
                auto jresult = (jstring)env->CallStaticObjectMethod(cls, mid, jsql, jdb);
                if (jni_check(env, "CallStaticObjectMethod(process)") && jresult) {
                    const char* result = env->GetStringUTFChars(jresult, nullptr);
                    printf("SqlEcho result:\n%s\n", result);
                    env->ReleaseStringUTFChars(jresult, result);
                }
            } else {
                // Legacy Helloworld path
                if (env->ExceptionCheck()) env->ExceptionClear();

                mid = env->GetMethodID(cls, "<init>", "()V");
                if (!jni_check(env, "GetMethodID(<init>)")) {
                    jvm->DestroyJavaVM();
                    return -1;
                }
                if (mid != nullptr) {
                    jobj = env->NewObject(cls, mid);
                    if (!jni_check(env, "NewObject")) {
                        jvm->DestroyJavaVM();
                        return -1;
                    }
                    std::cout << "init ok" << std::endl;
                }

                mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");
                if (!jni_check(env, "GetStaticMethodID(main)")) {
                    jvm->DestroyJavaVM();
                    return -1;
                }
                if (mid != nullptr) {
                    env->CallStaticVoidMethod(cls, mid, nullptr);
                    jni_check(env, "CallStaticVoidMethod(main)");
                }
            }
        } else {
            fprintf(stderr, "FindClass failed\n");
        }

        jvm->DestroyJavaVM();
        fprintf(stdout, "Java VM destroy.\n");
        return 0;
    } else {
        printf("create java jvm fail\n");
        return -1;
    }
}
