//
// Created by wanhui on 10/12/19.
//

#include <cstdio>
#include <jni.h>
#include <pthread.h>


#define NUM_THREADS 6

pthread_mutex_t mutexjvm;

pthread_t threads[NUM_THREADS];


struct JVM {
    JavaVM *jvm;
};

void invoke_class(JNIEnv* env);

void* jvmThreads(void *myJvm)
{
    auto* myJvmptr = (struct JVM*) myJvm;
    JavaVM *jvmPtr = myJvmptr->jvm;

    JNIEnv* env = nullptr;

    // Each thread will do following:
    // 1. lock the mutex,
    // 2. give some info to the user,
    // 3. attach to JVM,
    // 4. do JVM based stuff,
    // 5. detach from JVM,
    // 6. release mutex,
    // 7. finish thread's job.
    // Done
    pthread_mutex_lock(&mutexjvm);
    printf("I will call JVM\n");
    jvmPtr->AttachCurrentThread((void**)&(env), nullptr);
    invoke_class(env);
    jvmPtr->DetachCurrentThread();
    pthread_mutex_unlock(&mutexjvm);
    pthread_exit(nullptr);
}


JNIEnv *create_vm (struct JVM *jvm) {
    JNIEnv *env;
    JavaVMInitArgs vm_args;
    JavaVMOption options[3];

    options[0].optionString = "-Djava.compiler=NONE";
    options[1].optionString = "-Djava.class.path=.:/home/wanhui/Documents/callJvmThreadpool/qin_test.jar";
    options[2].optionString = "-verbose:jni";

    vm_args.options = options;
    vm_args.nOptions = 3;
    vm_args.ignoreUnrecognized = 0;
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

    Main_class = env->FindClass ("com/testjvm/Helloworld");
    if(Main_class == nullptr)
        return;
    fun_id = env->GetStaticMethodID (Main_class, "main", "([Ljava/lang/String;)V");

    env->CallStaticIntMethod (Main_class, fun_id);
}

int main () {
    struct JVM myJvm{};
    JNIEnv *myEnv = create_vm (&myJvm);

    if (myEnv == nullptr)
        return 1;

    pthread_mutex_init(&mutexjvm, nullptr);
    for(unsigned long & thread : threads)
    {
        pthread_create(&thread, nullptr, jvmThreads, (void*)&myJvm);
        pthread_join(thread, nullptr);
    }

    myJvm.jvm->DestroyJavaVM ();

    return 0;
}