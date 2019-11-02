//
// Created by wanhui on 10/12/19.
//

#include <unistd.h>
#include <cstdlib>
#include <jni.h>
#include "tpool.h"


#define NUM_THREADS 6


struct JVM {
    JavaVM *jvm;
};

void invoke_class(JNIEnv* env);

void* jvmThreads(void *myJvm)
{
    auto* myJvmptr = (struct JVM*) myJvm;
    JavaVM *jvmPtr = myJvmptr->jvm;

    JNIEnv* env = nullptr;

    jvmPtr->AttachCurrentThread((void**)&(env), nullptr);
    invoke_class(env);
    jvmPtr->DetachCurrentThread();

    return nullptr;
}


JNIEnv *create_vm (struct JVM *jvm) {
    JNIEnv *env;
    JavaVMInitArgs vm_args;
    JavaVMOption options[3];

    options[0].optionString = "-Djava.compiler=NONE";
    options[1].optionString = "-Djava.class.path=.:/home/wanhui/CallJvm/callJvmThreadpool/qin_test1.jar";
    options[2].optionString = "-verbose:jni";

    vm_args.options = options;
    vm_args.nOptions = 3;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    vm_args.version = JNI_VERSION_1_8;

    int status = JNI_CreateJavaVM (&jvm->jvm, (void **) &env, &vm_args);
    if (status < 0 || !env) {
        printf ("Error: %d\n", status);
        return nullptr;
    }
    return env;
}


void invoke_class (JNIEnv * env) {
    jclass Main_class;
    jmethodID fun_id;
    jmethodID static_id;
    jmethodID stu_id;
    jmethodID hello_id;
    jobject obj1;

    Main_class = env->FindClass ("com/testjvm/Helloworld");
    if(Main_class == nullptr)
        return;

    // test static function main()
//    fun_id = env->GetStaticMethodID (Main_class, "main", "([Ljava/lang/String;)V");
//    jstring str = env->NewStringUTF("XWH");
//    env->CallStaticVoidMethod(Main_class, fun_id, str);

    // test static function name()
//    static_id = env->GetStaticMethodID(Main_class, "name", "([Ljava/lang/String;)V");
//    jstring str = env->NewStringUTF("XWH");
//    env->CallStaticVoidMethod(Main_class, static_id, str);

    // test generally function student()
    hello_id = env->GetMethodID(Main_class, "<init>", "()V");
    obj1 = env->NewObject(Main_class, hello_id);
    jstring name = env->NewStringUTF("XWH");
    stu_id = env->GetMethodID(Main_class, "student", "([Ljava/lang/String;)V");
    env->CallObjectMethod(obj1, stu_id, name);
}

int main () {
    struct JVM myJvm{};
    JNIEnv *myEnv = create_vm (&myJvm);

    if (myEnv == nullptr)
        return 1;

    if(tpool_create(NUM_THREADS) != 0)
    {
        printf("tpool_create failed\n");
        exit(1);
    }

    int i;
    for(i = 0; i < 10; i++)
    {
        tpool_add_work(reinterpret_cast<void *(*)(void *)>(jvmThreads), reinterpret_cast<void *>(&myJvm));
    }

    sleep(2);
    tpool_destroy();
    myJvm.jvm->DestroyJavaVM ();

    return 0;
}
