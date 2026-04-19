# CallJvm

**Call JVM from C/C++ in a Thread Pool via JNI**

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-green)

## Overview

CallJvm demonstrates how to invoke Java code from C++ through the Java Native Interface (JNI). A naive JNI call creates and destroys a JVM per invocation, which is expensive. This project explores progressively better strategies: single JVM, multithreaded JVM sharing, and finally a POSIX thread pool that keeps worker threads attached to a long-lived JVM, amortizing the startup cost across many requests.

A socket-based server variant accepts work over TCP so external clients can trigger JVM calls without embedding JNI in their own processes.

## Features

- **Thread pool** (`tpool.cpp/h`) -- lightweight C-style POSIX thread pool with work queue
- **Modern C++ thread pool** (`threadpool.h`) -- header-only pool using `std::thread`, `std::mutex`, `std::condition_variable`, `std::queue<std::function<void()>>`; includes C API compatibility wrappers
- **Multithreaded JVM invocation** -- `AttachCurrentThread` / `DetachCurrentThread` for safe concurrent access
- **Socket server** -- TCP listener that dispatches incoming requests to the thread pool
- **Unix Domain Socket variant** (`udsThreadpool.cpp`) -- `AF_UNIX`/`SOCK_STREAM` IPC with length-prefixed framing for lower overhead than TCP
- **Length-prefixed framing** (`frame.h/cpp`) -- 4-byte big-endian length prefix for reliable message boundaries
- **RAII guards** (`jni_guard.h`) -- `JvmGuard` and `JniEnvGuard` for exception-safe JVM/thread lifecycle
- **JNI exception helper** (`jni_util.h`) -- `jni_check()` called after every JNI operation for reliable error reporting
- **Real-world Java example** (`SqlEcho.java`) -- parses SQL SELECT statements, returns structured JSON-like results via JNI
- **Benchmark** (`benchmark.cpp`) -- `std::chrono` timing harness comparing dispatch strategies
- **Tests** -- GoogleTest suite for thread pool and message parsing
- **Fuzz testing** (`fuzz/fuzz_frame.cpp`) -- libFuzzer target for the frame parser
- **CI** -- GitHub Actions: build matrix (JDK 17+21), sanitizers (ASan/UBSan/TSan), fuzzing, CodeQL
- **Release automation** -- prebuilt binaries for macOS arm64 and Linux x64 on tag push
- **Security** -- `SECURITY.md` with responsible disclosure policy

## Project Structure

```
callJvmThreadpool/
  CMakeLists.txt            # CMake build definition
  tpool.h / tpool.cpp       # C-style thread pool (POSIX pthreads)
  threadpool.h              # Modern C++ thread pool (header-only)
  jni_util.h                # JNI exception checking helper
  jni_guard.h               # RAII guards for JVM and JNI env
  frame.h / frame.cpp       # Length-prefixed framing protocol
  socketThreadpool.cpp      # TCP socket server + C thread pool + JNI (main target)
  udsThreadpool.cpp         # Unix Domain Socket server + C++ pool + framing + JNI
  socketMultithread.cpp     # TCP socket server + raw pthreads + JNI
  pureMultithread.cpp       # Standalone multithreaded JNI example
  main.cpp                  # Minimal single-thread JNI test
  benchmark.cpp             # Dispatch overhead benchmark
  server.cpp                # Standalone socket server
  client.cpp                # Socket client for testing
  test.cpp                  # Misc test code
  java/                     # Java examples
    build.sh                # Build script for Java classes
    src/com/xwhqsj/example/
      SqlEcho.java          # SQL-echo demo (parses SELECT, returns structured result)
  qin_test.jar              # Legacy test Java classes
  qin_test1.jar             # Legacy test Java classes (alternate)
tests/
  CMakeLists.txt
  tpool_test.cpp            # GoogleTest tests for tpool + parsing
fuzz/
  fuzz_frame.cpp            # libFuzzer target for frame parser
benchmarks/
  results.md                # Benchmark measurement template
.github/workflows/
  ci.yml                    # CI: build, test, sanitizers (ASan/UBSan/TSan), fuzz
  release.yml               # Release: prebuilt binaries on tag push
  codeql.yml                # CodeQL static analysis
.github/
  dependabot.yml            # Weekly GitHub Actions dependency updates
SECURITY.md                 # Security policy and responsible disclosure
```

## Requirements

- C++17 compiler (GCC, Clang, or MSVC)
- CMake 3.15+
- JDK with `JAVA_HOME` set (JNI headers and `libjvm` must be findable by CMake)
- Linux or macOS (POSIX sockets, pthreads)

## Build

```bash
# macOS: ensure JAVA_HOME is set
export JAVA_HOME=$(/usr/libexec/java_home)

# Configure and build
cmake -B build -S callJvmThreadpool
cmake --build build -j
```

CMake will locate JNI via `find_package(JNI REQUIRED)`.

## Configuration

Set `CALLJVM_CLASSPATH` to point to your jar files:

```bash
export CALLJVM_CLASSPATH="./qin_test1.jar:./qin_test.jar"
```

If not set, defaults to `"."` (current directory).

## Java Example: SqlEcho

The project includes a real-world Java example that demonstrates non-trivial JNI integration. `SqlEcho` accepts a SQL-like SELECT statement and a database name, parses the query using regex, and returns a structured JSON-like result.

```bash
# Build the Java example
cd callJvmThreadpool/java && ./build.sh && cd ../..

# Set classpath to the built JAR
export CALLJVM_CLASSPATH=callJvmThreadpool/java/sqlecho.jar

# Test standalone
java -cp callJvmThreadpool/java/sqlecho.jar com.xwhqsj.example.SqlEcho

# Run via JNI
./build/jni_test
```

The C++ code auto-detects `SqlEcho` on the classpath and falls back to the legacy `Helloworld` stub if not found.

## Run

```bash
# Thread-pool socket server (default target)
./build/main

# In another terminal, send a request
./build/client

# Unix Domain Socket variant
./build/uds_threadpool
```

The TCP server listens on port 8080. The UDS variant listens on `/tmp/calljvm.sock`. Both accept `$`-delimited payloads (plain SQL + DB name).

## Unix Domain Socket Variant

The `uds_threadpool` target uses `AF_UNIX`/`SOCK_STREAM` instead of TCP. Benefits:

- **Lower IPC overhead**: no TCP/IP stack processing, no port allocation
- **No network exposure**: socket file is local-only, inherits filesystem permissions
- **Better throughput for local IPC**: kernel-level shortcut avoids serialization/checksumming
- **Length-prefixed framing**: 4-byte big-endian length header ensures reliable message boundaries (no delimiter-based parsing)

Use this variant when the client and server are on the same machine.

## Benchmarks

Run the dispatch overhead benchmark:

```bash
./build/benchmark [iterations]
```

Example output (100 iterations, 6 pool threads):

```
=== CallJvm Dispatch Benchmark ===
Iterations: 100, Pool threads: 6

Strategy                                   Total (ms)     Avg (us)
---------------------------------------- ------------ ------------
tpool (C threadpool)                             1.23        12.30
raw pthreads (create+join per task)              8.45        84.50
std::thread (create+join per task)               7.91        79.10
```

The thread pool amortizes thread creation cost. Real JNI overhead (AttachCurrentThread + Java method call) is additional.

## Tests

Tests use GoogleTest (fetched automatically via CMake FetchContent):

```bash
cmake -B build -S callJvmThreadpool -DCALLJVM_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Test coverage:
- Thread pool: work items all execute, destroy joins correctly, null routine rejected
- Message parsing: normal delimiters, missing delimiters, empty input

## Limitations

- **Single JVM shared across threads**: all worker threads must call `AttachCurrentThread` before JNI operations and `DetachCurrentThread` after. The `JniEnvGuard` RAII wrapper handles this automatically.
- **JVM is single-instance per process**: JNI spec allows only one `JNI_CreateJavaVM` call per process.
- **strtok is not thread-safe**: the message parsing uses `strtok` on stack-local buffers, which is safe per-thread but the pattern is fragile. The UDS variant with framing is preferred.
- **macOS build note**: `JAVA_HOME` must be set. Use `export JAVA_HOME=$(/usr/libexec/java_home)`. If the JDK is not installed, the build will fail at `find_package(JNI)`. The CI matrix covers Linux with Temurin JDK 17 and 21.

## Design Notes

The project evolved through several stages:

1. **Single JVM** (`main.cpp`) -- create a JVM, call a Java method, destroy the JVM. Simple but slow per call.
2. **Multithread** (`pureMultithread.cpp`, `socketMultithread.cpp`) -- share one JVM across threads using `AttachCurrentThread`. Reduces JVM creation overhead.
3. **Thread pool** (`socketThreadpool.cpp` + `tpool.*`) -- pre-spawn worker threads, reuse them for incoming socket connections. Avoids per-request thread creation.
4. **Modern C++ pool** (`threadpool.h`) -- `std::thread`-based pool with `std::function<void()>` work items, replacing raw pthread + linked-list queue.
5. **UDS + framing** (`udsThreadpool.cpp` + `frame.*`) -- Unix Domain Socket with length-prefixed protocol for local IPC, eliminating TCP overhead and delimiter parsing fragility.

## References

- [Java Native Interface -- NTU Tutorial](https://www3.ntu.edu.sg/home/ehchua/programming/java/JavaNativeInterface.html)
- [JNI Specification -- Oracle](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html)

## License

[MIT](LICENSE)
