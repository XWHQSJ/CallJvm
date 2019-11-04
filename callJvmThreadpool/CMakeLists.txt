cmake_minimum_required(VERSION 3.14)
project(callJvmThreadpool)

set(CMAKE_CXX_STANDARD 14)

include_directories(/usr/local/java/include /usr/local/java/include/linux)

add_definitions(
        -O3 -g -W -Wall
        -Wunused-variable -Wunused-parameter -Wunused-function -Wunused
        -Wno-deprecated -Woverloaded-virtual -Wwrite-strings
        -D__WUR= -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DTIXML_USE_STL
)

add_executable(main jni.h jni_md.h tpool.cpp tpool.h
#                test.cpp
#                main.cpp
#                multithread.cpp
                threadpool.cpp
#                server.cpp
              )

target_link_libraries(main /usr/local/java/jre/lib/amd64/server/libjvm.so /usr/lib/x86_64-linux-gnu/libpthread.so)

