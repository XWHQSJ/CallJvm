#include <iostream>
#include "jni.h"

using namespace std;

int main()
{
    JavaVMOption options[3];
    JNIEnv *env;
    JavaVM *jvm;
    JavaVMInitArgs vm_args;
    long status;
    jclass cls;
    jmethodID mid;
    jint square;
    jobject jobj;
    options[0].optionString = "-Djava.compiler=NONE";
    options[1].optionString = "-Djava.class.path=.:/home/wanhui/Documents/callJvmThreadpool/qin_test.jar";
    options[2].optionString = "-verbose:jni"; //用于跟踪运行时的信息
    vm_args.version = JNI_VERSION_1_8; // JDK版本号
    vm_args.nOptions = 3;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    status = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);

    if(status != JNI_ERR){
        printf("create java jvm success\n");
        printf("%d\n", env);
        cls = env->FindClass("com/testjvm/Helloworld");  // 在这里查Java类
        printf("%d\n", cls);
        if(cls !=0){
            printf("find java class success\n");
            // 构造函数
            mid = env->GetMethodID(cls,"<init>","()V");
            if(mid !=0){
                jobj=env->NewObject(cls,mid);
                std::cout << "init ok" << std::endl;
            }

            // 调用main函数
            mid = env->GetStaticMethodID( cls, "main", "([Ljava/lang/String;)V");
            if(mid !=0){
                square = env->CallStaticIntMethod( cls, mid);
                std::cout << square << std::endl;
            }

        }
        else{
            fprintf(stderr, "FindClass failed\n");
        }

        jvm->DestroyJavaVM();
        fprintf(stdout, "Java VM destory.\n");
        return 0;
    }
    else{
        printf("create java jvm fail\n");
        return -1;
    }
}


