//
// Created by wanhui on 10/12/19.
//

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <jni.h>
#include "tpool.h"


#define NUM_THREADS 6

#define PORT 8080


struct JVM {
    JavaVM *jvm;
};

struct ARGS
{
    struct JVM* jvm;
    int socket;
};


void *jvmThreads(void *myJvm, char* plainsql, char* dbname);

JNIEnv *create_vm(struct JVM *jvm);

void invoke_class(JNIEnv* env, char* plainsql, char* dbname);

int socket_init();

void* handle_stream(void* arg);



int socket_init()
{
    int server_fd, new_socket;
    struct sockaddr_in address;

    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("listening...\n");
    if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    return new_socket;
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

void* handle_stream(void* args)
{
    struct ARGS *arg = static_cast<ARGS *>(args);
    struct JVM* myJvm = arg->jvm;
    int client_fd = arg->socket;

    char buf[1024] = {0};
    char* psql;
    char* dbn;
    char delims[] = "$";
    char *res = nullptr;
    std::vector<char*> resvec;

    read(client_fd, buf, 1024);

    res = strtok(buf, delims);
    while (res != nullptr)
    {
        resvec.push_back(res);
        res = strtok(nullptr, delims);
    }

    psql = resvec[0];
    dbn = resvec[1];

    jvmThreads(myJvm, psql, dbn);

    char hello[] = "Hello send";
    send(client_fd, hello, strlen(hello), 0);
    close(client_fd);
}

void* jvmThreads(void *myJvm, char* plainsql, char* dbname)
{
    auto* myJvmptr = (struct JVM*) myJvm;
    JavaVM *jvmPtr = myJvmptr->jvm;

    JNIEnv* env = nullptr;

    jvmPtr->AttachCurrentThread((void**)&(env), nullptr);
    invoke_class(env, plainsql, dbname);
    jvmPtr->DetachCurrentThread();

    return nullptr;
}

void invoke_class (JNIEnv * env, char* plainsql, char* dbname) {
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
    jstring plainsqlstr = env->NewStringUTF(plainsql);
    jstring dbnamestr = env->NewStringUTF(dbname);

    stu_id = env->GetMethodID(Main_class, "student", "([Ljava/lang/String;)V");
    env->CallObjectMethod(obj1, stu_id, dbnamestr);
}

int main () {
    struct JVM myJvm{};
    JNIEnv *myEnv = create_vm (&myJvm);

    if (myEnv == nullptr)
    {
        printf("create_vm failed\n");
        exit(1);
    }

    if(tpool_create(NUM_THREADS) != 0)
    {
        printf("tpool_create failed\n");
        exit(1);
    }


    int client_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    client_fd = socket_init();

    int i = 0;
    while (1){
        new_socket = accept(client_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen);

        struct ARGS *args;
        args = static_cast<ARGS *>(malloc(sizeof(struct args *)));
        args->jvm = &myJvm;
        args->socket = new_socket;

        tpool_add_work(handle_stream, args);
    }




//    handle_stream(args);

//    while (1)
//    {
//
//
//        tpool_add_work(handle_stream, args);
//    }



    // only 10 tasks

//    int i;
//    for(i = 0; i < 10; i++)
//    {
//        tpool_add_work(handle_stream, args);
//    }


    sleep(2);
    tpool_destroy();
    myJvm.jvm->DestroyJavaVM ();


    return 0;
}
