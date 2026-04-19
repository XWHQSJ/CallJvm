//
// udsThreadpool.cpp — Unix Domain Socket variant of socketThreadpool.cpp
//
// Uses AF_UNIX / SOCK_STREAM instead of TCP. Socket path: /tmp/calljvm.sock
// Uses length-prefixed framing (frame.h) for reliable message boundaries.
// Uses the modern C++ threadpool (threadpool.h) for dispatch.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <csignal>
#include <atomic>
#include <vector>
#include <jni.h>

#include "jni_util.h"
#include "jni_guard.h"
#include "threadpool.h"
#include "frame.h"

static constexpr int NUM_THREADS = 6;
static constexpr const char* SOCKET_PATH = "/tmp/calljvm.sock";

static std::atomic<bool> g_running{true};

static void signal_handler(int) {
    g_running.store(false);
}

static const char* get_classpath() {
    const char* cp = getenv("CALLJVM_CLASSPATH");
    return cp ? cp : ".";
}

struct JVM {
    JavaVM* jvm;
};

struct ARGS {
    JVM* jvm;
    int socket;
};

static JNIEnv* create_vm(JVM* jvm) {
    JNIEnv* env;
    JavaVMInitArgs vm_args;
    JavaVMOption options[3];

    std::string cp_opt = std::string("-Djava.class.path=") + get_classpath();

    options[0].optionString = const_cast<char*>("-Djava.compiler=NONE");
    options[1].optionString = const_cast<char*>(cp_opt.c_str());
    options[2].optionString = const_cast<char*>("-verbose:jni");

    vm_args.options = options;
    vm_args.nOptions = 3;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    vm_args.version = JNI_VERSION_1_8;

    int status = JNI_CreateJavaVM(&jvm->jvm, (void**)&env, &vm_args);
    if (status < 0 || !env) {
        fprintf(stderr, "JNI_CreateJavaVM failed: %d\n", status);
        return nullptr;
    }
    return env;
}

static void invoke_class(JNIEnv* env, const char* plainsql, const char* dbname) {
    jclass Main_class = env->FindClass("com/testjvm/Helloworld");
    if (!jni_check(env, "FindClass") || Main_class == nullptr)
        return;

    jmethodID init_id = env->GetMethodID(Main_class, "<init>", "()V");
    if (!jni_check(env, "GetMethodID(<init>)") || init_id == nullptr)
        return;

    jobject obj = env->NewObject(Main_class, init_id);
    if (!jni_check(env, "NewObject"))
        return;

    jstring dbnamestr = env->NewStringUTF(dbname);

    jmethodID stu_id = env->GetMethodID(Main_class, "student", "([Ljava/lang/String;)V");
    if (!jni_check(env, "GetMethodID(student)") || stu_id == nullptr)
        return;

    env->CallObjectMethod(obj, stu_id, dbnamestr);
    jni_check(env, "CallObjectMethod(student)");

    (void)plainsql;
}

static void handle_client(JVM* myJvm, int client_fd) {
    char buf[4096] = {0};
    int len = read_frame(client_fd, buf, sizeof(buf) - 1);
    if (len < 0) {
        fprintf(stderr, "read_frame failed\n");
        close(client_fd);
        return;
    }
    buf[len] = '\0';

    // Parse "$"-delimited payload: plainsql$dbname
    char* psql = nullptr;
    char* dbn = nullptr;
    char delims[] = "$";
    std::vector<char*> resvec;

    char* res = strtok(buf, delims);
    while (res != nullptr) {
        resvec.push_back(res);
        res = strtok(nullptr, delims);
    }

    if (resvec.size() < 2) {
        fprintf(stderr, "Invalid message: expected '$' delimiter\n");
        const char* err = "ERROR: bad format";
        write_frame(client_fd, err, static_cast<uint32_t>(strlen(err)));
        close(client_fd);
        return;
    }

    psql = resvec[0];
    dbn = resvec[1];

    // Attach thread to JVM, invoke, detach
    JavaVM* jvmPtr = myJvm->jvm;
    JNIEnv* env = nullptr;
    JniEnvGuard guard;
    guard.vm = jvmPtr;

    jint rc = jvmPtr->AttachCurrentThread((void**)&env, nullptr);
    if (rc != JNI_OK || !env) {
        fprintf(stderr, "AttachCurrentThread failed\n");
        close(client_fd);
        return;
    }
    guard.attached = true;

    invoke_class(env, psql, dbn);

    const char* reply = "OK";
    write_frame(client_fd, reply, static_cast<uint32_t>(strlen(reply)));
    close(client_fd);
}

static int uds_init() {
    unlink(SOCKET_PATH);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket(AF_UNIX) failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("UDS listening on %s\n", SOCKET_PATH);
    return server_fd;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    JVM myJvm{};
    JNIEnv* myEnv = create_vm(&myJvm);
    if (myEnv == nullptr) {
        fprintf(stderr, "create_vm failed\n");
        return 1;
    }

    ThreadPool pool(NUM_THREADS);

    int server_fd = uds_init();

    while (g_running.load()) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (!g_running.load()) break;
            perror("accept failed");
            continue;
        }

        pool.submit([&myJvm, client_fd]() {
            handle_client(&myJvm, client_fd);
        });
    }

    printf("Shutting down UDS server...\n");
    close(server_fd);
    unlink(SOCKET_PATH);
    pool.destroy();
    myJvm.jvm->DestroyJavaVM();

    return 0;
}
