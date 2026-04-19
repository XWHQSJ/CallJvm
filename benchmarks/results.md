# CallJvm Benchmark Results

## Machine Specs

| Item | Value |
|------|-------|
| OS | macOS (Apple Silicon) |
| CPU | Apple M-series (arm64) |
| RAM | TBD |
| JDK | Temurin 21 (arm64) |
| Compiler | Apple Clang (C++17) |
| CMake | 3.15+ |

## How to Reproduce

```bash
# Build
export JAVA_HOME=$(/usr/libexec/java_home)
cmake -B build -S callJvmThreadpool -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --config Release

# Run benchmark (C++ dispatch overhead only)
./build/benchmark 100
./build/benchmark 1000
./build/benchmark 10000
```

For full JNI benchmarks, build the Java example and set the classpath:

```bash
cd callJvmThreadpool/java && ./build.sh && cd ../..
export CALLJVM_CLASSPATH=callJvmThreadpool/java/sqlecho.jar
```

## C++ Dispatch Overhead

Measures thread dispatch cost only (no JVM). Pool size: 6 threads.

### N = 100

| Strategy | Total (ms) | Avg (us) |
|----------|-----------|----------|
| tpool (C threadpool) | TBD | TBD |
| raw pthreads (create+join) | TBD | TBD |
| std::thread (create+join) | TBD | TBD |

### N = 1,000

| Strategy | Total (ms) | Avg (us) |
|----------|-----------|----------|
| tpool (C threadpool) | TBD | TBD |
| raw pthreads (create+join) | TBD | TBD |
| std::thread (create+join) | TBD | TBD |

### N = 10,000

| Strategy | Total (ms) | Avg (us) |
|----------|-----------|----------|
| tpool (C threadpool) | TBD | TBD |
| raw pthreads (create+join) | TBD | TBD |
| std::thread (create+join) | TBD | TBD |

## Full JNI Benchmark (with JVM)

Requires JDK at runtime. Measures end-to-end: dispatch + AttachCurrentThread + Java invocation.

### Socket Variants (N = 1,000)

| Strategy | Latency Mean (us) | P50 (us) | P95 (us) | P99 (us) | Throughput (req/s) |
|----------|-------------------|----------|----------|----------|--------------------|
| pureMultithread | TBD | TBD | TBD | TBD | TBD |
| socketMultithread (TCP) | TBD | TBD | TBD | TBD | TBD |
| socketThreadpool (TCP) | TBD | TBD | TBD | TBD | TBD |
| udsThreadpool (UDS) | TBD | TBD | TBD | TBD | TBD |

### Expected Relative Performance

Based on architecture:

- **UDS vs TCP loopback**: UDS bypasses the TCP/IP stack entirely (no checksumming, no port allocation, no congestion control). Typical savings: ~20-40% latency reduction for local IPC.
- **Threadpool vs spawn-per-request**: Pre-spawned pool avoids `pthread_create`/`pthread_join` per request. At high N, threadpool throughput can be 5-10x higher due to amortized thread creation cost.
- **Framed protocol vs raw read**: Length-prefixed framing eliminates delimiter parsing and partial-read bugs, with negligible overhead (4 bytes per message).

## How to Populate

Replace TBD values by running the benchmarks on your target machine:

1. Build in Release mode as shown above.
2. Run `./build/benchmark N` for each N value.
3. For socket/UDS variants, start the server, then use a load-generation script.
4. Record mean, P50, P95, P99 from the output.
5. Calculate throughput as `N / total_seconds`.
