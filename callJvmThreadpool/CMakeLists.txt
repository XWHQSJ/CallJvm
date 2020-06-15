cmake_minimum_required(VERSION 3.14)
project(callJvmThreadpool)

set(CMAKE_CXX_STANDARD 14)

# 运行环境配置的java运行环境路径
include_directories(/usr/local/java/include /usr/local/java/include/linux)

# 编译选项（可不修改）
add_definitions(
        -O3 -g -W -Wall
        -Wunused-variable -Wunused-parameter -Wunused-function -Wunused
        -Wno-deprecated -Woverloaded-virtual -Wwrite-strings
        -D__WUR= -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DTIXML_USE_STL
)

# 编译文件
# 除jni.h jni_md.h tpool.cpp tpool.h文件外，
# 下面每个文件都可选择性编译（含有main函数的文件不可同时编译）
add_executable(main jni.h jni_md.h tpool.cpp tpool.h
#                test.cpp
#                main.cpp
#        socketMultithread.cpp
#                server.cpp
        socketThreadpool.cpp
        )


# 链接的动态库，包括java的libjvm.so文件和linux多线程的libpthread.so文件
target_link_libraries(main /usr/local/java/jre/lib/amd64/server/libjvm.so /usr/lib/x86_64-linux-gnu/libpthread.so)

