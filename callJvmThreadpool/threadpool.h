#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Modern C++ thread pool using std::thread, std::mutex, std::condition_variable.
// The old C API (tpool_create/tpool_destroy/tpool_add_work) is kept as
// compatibility wrappers below.

class ThreadPool {
public:
    explicit ThreadPool(int num_threads) : shutdown_(false) {
        workers_.reserve(num_threads);
        for (int i = 0; i < num_threads; i++) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~ThreadPool() { destroy(); }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit a callable work item.
    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            queue_.push(std::move(task));
        }
        cv_.notify_one();
    }

    // Submit a C-style work function (for backward compatibility).
    void submit_c(void* (*routine)(void*), void* arg) {
        submit([routine, arg]() { routine(arg); });
    }

    // Graceful shutdown: signal all workers, join threads, drain remaining work.
    void destroy() {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (shutdown_) return;
            shutdown_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
        workers_.clear();
    }

    bool is_shutdown() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return shutdown_;
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(mutex_);
                cv_.wait(lk, [this] { return shutdown_ || !queue_.empty(); });
                if (shutdown_ && queue_.empty()) return;
                task = std::move(queue_.front());
                queue_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_;
};

// ---- C API compatibility wrappers (match tpool.h interface) ----

// Global instance used by the C API wrappers.
// Only one pool can be active at a time via these wrappers.
namespace detail {
inline ThreadPool*& global_pool() {
    static ThreadPool* pool = nullptr;
    return pool;
}
}  // namespace detail

inline int tpool_create_modern(int max_thr_num) {
    if (detail::global_pool()) return -1;
    detail::global_pool() = new ThreadPool(max_thr_num);
    return 0;
}

inline void tpool_destroy_modern() {
    if (detail::global_pool()) {
        detail::global_pool()->destroy();
        delete detail::global_pool();
        detail::global_pool() = nullptr;
    }
}

inline int tpool_add_work_modern(void* (*routine)(void*), void* arg) {
    if (!detail::global_pool() || !routine) return -1;
    detail::global_pool()->submit_c(routine, arg);
    return 0;
}
