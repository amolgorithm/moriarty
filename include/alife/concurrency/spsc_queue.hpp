#pragma once
#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace alife {

// Bounded lock-free ring, one producer and one consumer. Capacity is rounded up
// to a power of two. Intended for the micro to evo hand-off in phase 4.
template <class T>
class SpscQueue {
   public:
    explicit SpscQueue(std::size_t capacity) {
        std::size_t c = 2;
        while (c < capacity) c <<= 1;
        buf_.resize(c);
        mask_ = c - 1;
    }

    bool push(T v) {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        if (h - tail_.load(std::memory_order_acquire) == buf_.size()) return false;
        buf_[h & mask_] = std::move(v);
        head_.store(h + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) return std::nullopt;
        T v = std::move(buf_[t & mask_]);
        tail_.store(t + 1, std::memory_order_release);
        return v;
    }

   private:
    std::vector<T> buf_;
    std::size_t mask_ = 0;
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

}  // namespace alife
