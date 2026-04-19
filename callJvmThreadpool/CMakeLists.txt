cmake_minimum_required(VERSION 3.15)
project(callJvmThreadpool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Locate JNI (requires JAVA_HOME to be set)
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# Common compile options
add_compile_options(
    -O2 -g -Wall -Wextra
    -Wunused-variable -Wunused-parameter -Wunused-function
    -Wno-deprecated -Woverloaded-virtual -Wwrite-strings
)

# Threads (pthreads)
find_package(Threads REQUIRED)

# Shared sources (thread pool)
set(TPOOL_SOURCES tpool.cpp tpool.h)

# Default target: socket server with thread pool + JNI
add_executable(main ${TPOOL_SOURCES} socketThreadpool.cpp)
target_link_libraries(main PRIVATE ${JNI_LIBRARIES} Threads::Threads)

# Socket multithread variant (raw pthreads)
add_executable(socket_multithread socketMultithread.cpp)
target_link_libraries(socket_multithread PRIVATE ${JNI_LIBRARIES} Threads::Threads)

# Pure multithread variant
add_executable(pure_multithread pureMultithread.cpp)
target_link_libraries(pure_multithread PRIVATE ${JNI_LIBRARIES} Threads::Threads)

# Minimal single-thread JNI test
add_executable(jni_test main.cpp)
target_link_libraries(jni_test PRIVATE ${JNI_LIBRARIES})

# Standalone socket server
add_executable(server server.cpp)
target_link_libraries(server PRIVATE Threads::Threads)

# Socket client
add_executable(client client.cpp)

# Benchmark
add_executable(benchmark ${TPOOL_SOURCES} benchmark.cpp)
target_link_libraries(benchmark PRIVATE Threads::Threads)

# Unix Domain Socket variant with framed protocol + modern C++ threadpool
add_executable(uds_threadpool frame.cpp udsThreadpool.cpp)
target_link_libraries(uds_threadpool PRIVATE ${JNI_LIBRARIES} Threads::Threads)

# --- Tests (optional, requires GoogleTest) ---
option(CALLJVM_BUILD_TESTS "Build tests" ON)
if(CALLJVM_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(../tests tests)
endif()
