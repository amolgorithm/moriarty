#include "alife/concurrency/thread_pool.hpp"

#include <algorithm>
#include <thread>

namespace alife {

ThreadPool::ThreadPool(std::size_t threads)
    : threads_(threads ? threads
                       : std::clamp<std::size_t>(std::thread::hardware_concurrency(), 1, 8)) {
    workers_.reserve(threads_ > 0 ? threads_ - 1 : 0);
    try {
        for (std::size_t i = 1; i < threads_; ++i) workers_.emplace_back(&ThreadPool::workerLoop, this);
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        workAvailable_.notify_all();
        for (auto& worker : workers_)
            if (worker.joinable()) worker.join();
        throw;
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    workAvailable_.notify_all();
    for (auto& worker : workers_)
        if (worker.joinable()) worker.join();
}

void ThreadPool::workerLoop() {
    std::size_t observedGeneration = 0;
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            workAvailable_.wait(lock, [this, observedGeneration] {
                return stopping_ || generation_ != observedGeneration;
            });
            if (stopping_) return;
            observedGeneration = generation_;
        }

        runChunks();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (--activeWorkers_ == 0) taskDone_.notify_one();
        }
    }
}

void ThreadPool::runChunks() noexcept {
    while (!aborted_.load(std::memory_order_relaxed)) {
        const std::size_t begin = next_.fetch_add(taskChunk_, std::memory_order_relaxed);
        if (begin >= taskSize_) break;
        try {
            task_(begin, std::min(taskSize_, begin + taskChunk_));
        } catch (...) {
            recordException(std::current_exception());
            break;
        }
    }
}

void ThreadPool::recordException(std::exception_ptr error) noexcept {
    aborted_.store(true, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(mutex_);
    if (!error_) error_ = std::move(error);
}

}  // namespace alife
