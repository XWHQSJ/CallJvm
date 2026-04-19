//
// benchmark.cpp — Timing harness for JNI invocation strategies.
//
// Compares the overhead of creating JVM, attaching threads, and invoking
// Java methods across the different strategies in this project.
// Since actual JVM invocations depend on the Java classpath being available,
// this benchmark measures the thread pool dispatch overhead (the C++ side).
//
// Usage:
//   ./benchmark [iterations]
//   Default: 100 iterations
//

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "tpool.h"

static const int DEFAULT_ITERATIONS = 100;
static const int NUM_THREADS = 6;

static std::mutex g_mutex;
static int g_counter = 0;

// Simulated work function for tpool (C API)
static void* tpool_work_fn(void* arg) {
    (void)arg;
    std::lock_guard<std::mutex> lk(g_mutex);
    g_counter++;
    return nullptr;
}

// Simulated work function for raw pthreads
static void* pthread_work_fn(void* arg) {
    (void)arg;
    std::lock_guard<std::mutex> lk(g_mutex);
    g_counter++;
    return nullptr;
}

struct BenchResult {
    const char* name;
    double total_ms;
    double avg_us;
    int iterations;
};

static BenchResult bench_tpool(int iterations) {
    g_counter = 0;
    tpool_create(NUM_THREADS);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        tpool_add_work(tpool_work_fn, nullptr);
    }
    // Wait for all work to complete by polling
    while (true) {
        std::lock_guard<std::mutex> lk(g_mutex);
        if (g_counter >= iterations) break;
    }
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    tpool_destroy();

    return {"tpool (C threadpool)", ms, (ms * 1000.0) / iterations, iterations};
}

static BenchResult bench_raw_pthreads(int iterations) {
    g_counter = 0;
    std::vector<pthread_t> threads(iterations);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        pthread_create(&threads[i], nullptr, pthread_work_fn, nullptr);
    }
    for (int i = 0; i < iterations; i++) {
        pthread_join(threads[i], nullptr);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return {"raw pthreads (create+join per task)", ms, (ms * 1000.0) / iterations, iterations};
}

static BenchResult bench_std_threads(int iterations) {
    g_counter = 0;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    threads.reserve(iterations);
    for (int i = 0; i < iterations; i++) {
        threads.emplace_back([]() {
            std::lock_guard<std::mutex> lk(g_mutex);
            g_counter++;
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return {"std::thread (create+join per task)", ms, (ms * 1000.0) / iterations, iterations};
}

int main(int argc, char* argv[]) {
    int iterations = DEFAULT_ITERATIONS;
    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0) iterations = DEFAULT_ITERATIONS;
    }

    printf("=== CallJvm Dispatch Benchmark ===\n");
    printf("Iterations: %d, Pool threads: %d\n\n", iterations, NUM_THREADS);

    BenchResult results[] = {
        bench_tpool(iterations),
        bench_raw_pthreads(iterations),
        bench_std_threads(iterations),
    };

    printf("%-40s %12s %12s\n", "Strategy", "Total (ms)", "Avg (us)");
    printf("%-40s %12s %12s\n", "----------------------------------------", "------------",
           "------------");
    for (const auto& r : results) {
        printf("%-40s %12.2f %12.2f\n", r.name, r.total_ms, r.avg_us);
    }

    printf("\nNote: This measures C++ dispatch overhead only.\n");
    printf("Real JNI overhead (AttachCurrentThread + Java call) is additional.\n");

    return 0;
}
