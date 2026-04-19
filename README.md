# CallJvm

**Call JVM from C/C++ in a Thread Pool via JNI**

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-green)

## Overview

CallJvm demonstrates how to invoke Java code from C++ through the Java Native Interface (JNI). A naive JNI call creates and destroys a JVM per invocation, which is expensive. This project explores progressively better strategies: single JVM, multithreaded JVM sharing, and finally a POSIX thread pool that keeps worker threads attached to a long-lived JVM, amortizing the startup cost across many requests.

A socket-based server variant accepts work over TCP so external clients can trigger JVM calls without embedding JNI in their own processes.

## Features

- **Thread pool** (`tpool.c/h`) -- lightweight C-style POSIX thread pool with work queue
- **Multithreaded JVM invocation** -- `AttachCurrentThread` / `DetachCurrentThread` for safe concurrent access
- **Socket server** -- TCP listener that dispatches incoming requests to the thread pool
- **Pure multithread baseline** -- standalone multithreaded JVM example for comparison
- **Unix Domain Socket note** -- the project README records a later realization that UDS-based IPC is more practical than JNI for production use

## Project Structure

```
callJvmThreadpool/
  CMakeLists.txt            # CMake build definition
  tpool.h / tpool.cpp       # Thread pool implementation
  socketThreadpool.cpp      # Socket server + thread pool + JNI (main target)
  socketMultithread.cpp     # Socket server + raw pthreads + JNI
  pureMultithread.cpp       # Standalone multithreaded JNI example
  main.cpp                  # Minimal single-thread JNI test
  server.cpp                # Standalone socket server
  client.cpp                # Socket client for testing
  test.cpp                  # Misc test code
  jni.h / jni_md.h          # Vendored JNI headers (Linux)
  qin_test.jar              # Test Java classes
  qin_test1.jar             # Test Java classes (alternate)
```

## Requirements

- C++17 compiler (GCC, Clang, or MSVC)
- CMake 3.15+
- JDK with `JAVA_HOME` set (JNI headers and `libjvm` must be findable by CMake)
- Linux or macOS (POSIX sockets, pthreads)

## Build

Out-of-source build:

```bash
cmake -S callJvmThreadpool -B build
cmake --build build
```

CMake will locate JNI via `find_package(JNI REQUIRED)`, so make sure `JAVA_HOME` points to your JDK installation.

## Run

Depending on which target was compiled (see `CMakeLists.txt` for the active `add_executable`):

```bash
# Thread-pool socket server (default target)
./build/main

# In another terminal, send a request
./build/client
```

The server listens on port 8080. The client sends a `$`-delimited payload (plain SQL + DB name) and receives a response.

For the standalone single-thread test (`main.cpp`), adjust the classpath in the source to point to your jar location, rebuild, and run directly.

## Design Notes

The project evolved through several stages:

1. **Single JVM** (`main.cpp`) -- create a JVM, call a Java method, destroy the JVM. Simple but slow per call.
2. **Multithread** (`pureMultithread.cpp`, `socketMultithread.cpp`) -- share one JVM across threads using `AttachCurrentThread`. Reduces JVM creation overhead.
3. **Thread pool** (`socketThreadpool.cpp` + `tpool.*`) -- pre-spawn worker threads, reuse them for incoming socket connections. Avoids per-request thread creation.
4. **UDS realization** -- invoking JNI still carries significant overhead per call. For production, the author notes that Unix Domain Sockets with file-backed data mapping is a more practical IPC strategy than embedding JNI directly.

## References

- [Java Native Interface -- NTU Tutorial](https://www3.ntu.edu.sg/home/ehchua/programming/java/JavaNativeInterface.html)
- [JNI Specification -- Oracle](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html)

## License

[MIT](LICENSE)
