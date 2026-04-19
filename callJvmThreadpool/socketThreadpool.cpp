//
// Created by wanhui on 10/12/19.
//

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>
#include <jni.h>
#include <pthread.h>
#include <csignal>
#include <atomic>
#include "jni_util.h"
#include "tpool.h"

#define NUM_THREADS 6
#define PORT 8080

static std::atomic<bool> g_running{true};

static void signal_handler(int) {
    g_running.store(false);
}

static const char* get_classpath() {
    const char* cp = getenv("CALLJVM_CLASSPATH");
    return cp ? cp : ".";
}

struct JVM {
    JavaVM *jvm;
};

struct ARGS {
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
    int server_fd;
    struct sockaddr_in address;

    int opt = 1;

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

    return server_fd;
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
    char *saveptr = nullptr;
    std::vector<char*> resvec;

    read(client_fd, buf, 1024);

    res = strtok_r(buf, delims, &saveptr);
    while (res != nullptr)
    {
        resvec.push_back(res);
        res = strtok_r(nullptr, delims, &saveptr);
    }

    if (resvec.size() < 2) {
        fprintf(stderr, "Invalid message: expected '$' delimiter\n");
        close(client_fd);
        free(arg);
        return nullptr;
    }

    psql = resvec[0];
    dbn = resvec[1];

    jvmThreads(myJvm, psql, dbn);

    char hello[] = "Hello send";
    send(client_fd, hello, strlen(hello), 0);
    close(client_fd);
    free(arg);
    return nullptr;
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

void invoke_class(JNIEnv *env, char* plainsql, char* dbname) {
    // Try SqlEcho first (new example), fall back to Helloworld (legacy stub)
    jclass cls = env->FindClass("com/xwhqsj/example/SqlEcho");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        // Fall back to legacy Helloworld class
        cls = env->FindClass("com/testjvm/Helloworld");
        if (!jni_check(env, "FindClass(Helloworld)") || cls == nullptr)
            return;

        jmethodID hello_id = env->GetMethodID(cls, "<init>", "()V");
        if (!jni_check(env, "GetMethodID(<init>)") || hello_id == nullptr)
            return;
        jobject obj1 = env->NewObject(cls, hello_id);
        if (!jni_check(env, "NewObject"))
            return;
        jstring dbnamestr = env->NewStringUTF(dbname);
        jmethodID stu_id = env->GetMethodID(cls, "student", "([Ljava/lang/String;)V");
        if (!jni_check(env, "GetMethodID(student)") || stu_id == nullptr)
            return;
        env->CallObjectMethod(obj1, stu_id, dbnamestr);
        jni_check(env, "CallObjectMethod(student)");
        (void)plainsql;
        return;
    }

    // SqlEcho.process(sql, dbName) -> String
    jmethodID process_id = env->GetStaticMethodID(
        cls, "process",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if (!jni_check(env, "GetStaticMethodID(process)") || process_id == nullptr)
        return;

    jstring jsql = env->NewStringUTF(plainsql);
    jstring jdb = env->NewStringUTF(dbname);

    auto jresult = (jstring)env->CallStaticObjectMethod(cls, process_id, jsql, jdb);
    if (!jni_check(env, "CallStaticObjectMethod(process)"))
        return;

    if (jresult) {
        const char* result = env->GetStringUTFChars(jresult, nullptr);
        printf("SqlEcho result:\n%s\n", result);
        env->ReleaseStringUTFChars(jresult, result);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    struct JVM myJvm{};
    JNIEnv *myEnv = create_vm(&myJvm);

    if (myEnv == nullptr) {
        printf("create_vm failed\n");
        exit(1);
    }

    if (tpool_create(NUM_THREADS) != 0) {
        printf("tpool_create failed\n");
        exit(1);
    }

    int client_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    client_fd = socket_init();

    while (g_running.load()) {
        new_socket = accept(client_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
        if (new_socket < 0) {
            if (!g_running.load())
                break;
            perror("accept failed");
            continue;
        }

        struct ARGS *args;
        args = static_cast<ARGS *>(malloc(sizeof(struct ARGS)));
        args->jvm = &myJvm;
        args->socket = new_socket;

        tpool_add_work(handle_stream, args);
    }

    printf("Shutting down...\n");
    close(client_fd);
    tpool_destroy();
    myJvm.jvm->DestroyJavaVM();

    return 0;
}
