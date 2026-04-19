//
// Created by wanhui on 10/12/19.
//

#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <jni.h>
#include "jni_util.h"

#define PORT 8080
#define NUM_THREADS 6

static const char* get_classpath() {
    const char* cp = getenv("CALLJVM_CLASSPATH");
    return cp ? cp : ".";
}

pthread_mutex_t mutexjvm;

pthread_t threads[NUM_THREADS];


struct JVM {
    JavaVM *jvm;
};

struct ARGS {
    struct JVM* jvm;
    int socket;
};

void *jvmThreads(void *args, char *plainsql, char *dbname);

void invoke_class(JNIEnv *env, char *plainsql, char *dbname);

int socket_init();

JNIEnv *create_vm(struct JVM *jvm);

void* handle_stream(void* arg);

void* handle_stream(void* args)
{
    printf("I am get in");

    struct ARGS *arg = static_cast<ARGS *>(args);
    struct JVM *myJvm = arg->jvm;
    int client_fd = arg->socket;
    char buf[1024] = {0};
    char hello[] = "Hello from server";

    read(client_fd, buf, 1024);

    char* psql;
    char* dbn;
    char delims[] = "$";
    char *res = nullptr;
    char *saveptr = nullptr;
    std::vector<char*> resvec;

    res = strtok_r(buf, delims, &saveptr);
    while (res != nullptr)
    {
        resvec.push_back(res);
        res = strtok_r(nullptr, delims, &saveptr);
    }

    if (resvec.size() < 2) {
        fprintf(stderr, "Invalid message: expected '$' delimiter\n");
        return nullptr;
    }

    psql = resvec[0];
    dbn = resvec[1];

    jvmThreads(myJvm, psql, dbn);

    send(client_fd, hello, strlen(hello), 0);
    printf("Hello message send");
    return nullptr;
}

void *jvmThreads(void *myJvm, char *plainsql, char *dbname) {
    auto *myJvmptr = (struct JVM *) myJvm;
    JavaVM *jvmPtr = myJvmptr->jvm;

    JNIEnv *env = nullptr;

    pthread_mutex_lock(&mutexjvm);
    printf("I will call JVM\n");
    jvmPtr->AttachCurrentThread((void **) &(env), nullptr);
    invoke_class(env, plainsql, dbname);
    jvmPtr->DetachCurrentThread();
    pthread_mutex_unlock(&mutexjvm);
    pthread_exit(nullptr);
}


JNIEnv *create_vm(struct JVM *jvm) {
    JNIEnv *env;
    JavaVMInitArgs vm_args;
    JavaVMOption options[3];

    std::string cp_opt = std::string("-Djava.class.path=") + get_classpath();

    options[0].optionString = const_cast<char *>("-Djava.compiler=NONE");
    options[1].optionString = const_cast<char *>(cp_opt.c_str());
    options[2].optionString = const_cast<char *>("-verbose:jni");

    vm_args.options = options;
    vm_args.nOptions = 3;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    vm_args.version = JNI_VERSION_1_8;

    int status = JNI_CreateJavaVM(&jvm->jvm, (void **) &env, &vm_args);
    if (status < 0 || !env) {
        printf("Error: %d\n", status);
        return nullptr;
    }
    return env;
}


void invoke_class(JNIEnv *env, char *plainsql, char *dbname) {
    jclass Main_class;
    jmethodID stu_id;
    jmethodID hello_id;
    jobject obj1;

    Main_class = env->FindClass("com/testjvm/Helloworld");
    if (!jni_check(env, "FindClass") || Main_class == nullptr)
        return;

    hello_id = env->GetMethodID(Main_class, "<init>", "()V");
    if (!jni_check(env, "GetMethodID(<init>)") || hello_id == nullptr)
        return;
    obj1 = env->NewObject(Main_class, hello_id);
    if (!jni_check(env, "NewObject"))
        return;
    jstring plainsqlstr = env->NewStringUTF(plainsql);
    jstring dbnamestr = env->NewStringUTF(dbname);

    stu_id = env->GetMethodID(Main_class, "student", "([Ljava/lang/String;)V");
    if (!jni_check(env, "GetMethodID(student)") || stu_id == nullptr)
        return;
    env->CallObjectMethod(obj1, stu_id, dbnamestr);
    jni_check(env, "CallObjectMethod(student)");

    (void)plainsqlstr;
}

int socket_init() {
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


int main() {
    struct JVM myJvm{};
    JNIEnv *myEnv = create_vm(&myJvm);

    if (myEnv == nullptr)
        return 1;

    int new_socket = socket_init();

    struct ARGS *args;
    args = static_cast<ARGS *>(malloc(sizeof(struct ARGS)));
    args->jvm = &myJvm;
    args->socket = new_socket;

    pthread_mutex_init(&mutexjvm, nullptr);
    for (pthread_t &thread : threads) {
        printf("before call");
        pthread_create(&thread, nullptr, handle_stream, (void *) args);
    }
    for (pthread_t &thread : threads) {
        pthread_join(thread, nullptr);
    }

    myJvm.jvm->DestroyJavaVM();

    return 0;
}
