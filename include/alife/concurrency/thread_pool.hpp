#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace alife {

class ThreadPool {
   public:
    explicit ThreadPool(std::size_t threads = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::size_t threadCount() const noexcept { return threads_; }

    template <class F>
    void parallelFor(std::size_t n, std::size_t chunk, F&& f) {
        if (chunk == 0) throw std::invalid_argument("parallelFor chunk must be greater than zero");
        if (n == 0) return;
        if (workers_.empty() || n <= chunk) {
            for (std::size_t begin = 0; begin < n; begin += chunk)
                f(begin, std::min(n, begin + chunk));
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            task_ = std::forward<F>(f);
            taskSize_ = n;
            taskChunk_ = chunk;
            next_.store(0, std::memory_order_relaxed);
            aborted_.store(false, std::memory_order_relaxed);
            error_ = nullptr;
            activeWorkers_ = workers_.size();
            ++generation_;
        }
        workAvailable_.notify_all();
        runChunks();

        std::exception_ptr error;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            taskDone_.wait(lock, [this] { return activeWorkers_ == 0; });
            error = error_;
            task_ = {};
        }
        if (error) std::rethrow_exception(error);
    }

   private:
    void workerLoop();
    void runChunks() noexcept;
    void recordException(std::exception_ptr error) noexcept;

    std::size_t threads_ = 1;
    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable workAvailable_, taskDone_;
    std::function<void(std::size_t, std::size_t)> task_;
    std::size_t taskSize_ = 0, taskChunk_ = 1, generation_ = 0, activeWorkers_ = 0;
    std::atomic<std::size_t> next_{0};
    std::atomic<bool> aborted_{false};
    std::exception_ptr error_;
    bool stopping_ = false;
};

}  // namespace alife
