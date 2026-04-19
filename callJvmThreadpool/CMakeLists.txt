cmake_minimum_required(VERSION 3.15)
project(callJvmThreadpool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate JNI (requires JAVA_HOME to be set)
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# Common compile options
add_compile_options(
    -O3 -g -Wall -Wextra
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

# Optional targets -- uncomment the ones you need.
# Only one main()-containing source can be active per target.

# add_executable(socket_multithread ${TPOOL_SOURCES} socketMultithread.cpp)
# target_link_libraries(socket_multithread PRIVATE ${JNI_LIBRARIES} Threads::Threads)

# add_executable(pure_multithread pureMultithread.cpp)
# target_link_libraries(pure_multithread PRIVATE ${JNI_LIBRARIES} Threads::Threads)

# add_executable(jni_test main.cpp)
# target_link_libraries(jni_test PRIVATE ${JNI_LIBRARIES})

# add_executable(server server.cpp)
# target_link_libraries(server PRIVATE Threads::Threads)

# add_executable(client client.cpp)
