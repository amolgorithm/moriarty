#pragma once
#include <algorithm>
#include <cstddef>

namespace alife {

// Phase 0 placeholder: runs serially. The interface is final, workers land in phase 4.
class ThreadPool {
   public:
    explicit ThreadPool(std::size_t threads = 0);
    std::size_t threadCount() const noexcept { return threads_; }

    // f(begin, end) over [0, n) in chunks
    template <class F>
    void parallelFor(std::size_t n, std::size_t chunk, F&& f) {
        // TODO(phase 4): dispatch chunks to workers, join with std::latch
        for (std::size_t b = 0; b < n; b += chunk) f(b, std::min(n, b + chunk));
    }

   private:
    std::size_t threads_ = 1;
};

}  // namespace alife
