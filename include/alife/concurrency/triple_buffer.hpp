#pragma once
#include <atomic>

namespace alife {

// Single producer, single consumer. Neither side blocks the other.
// Used to hand render snapshots from the sim to the viewer.
template <class T>
class TripleBuffer {
   public:
    // producer side
    T& back() noexcept { return buf_[back_]; }
    void publish() noexcept {
        const int old = mid_.exchange(back_ | kDirty, std::memory_order_acq_rel);
        back_ = old & 3;
    }

    // consumer side: latest published frame, or the previous one if nothing new
    const T& front() noexcept {
        if (mid_.load(std::memory_order_acquire) & kDirty) {
            const int old = mid_.exchange(front_, std::memory_order_acq_rel);
            front_ = old & 3;
        }
        return buf_[front_];
    }

   private:
    static constexpr int kDirty = 4;
    T buf_[3]{};
    int back_ = 0;
    int front_ = 2;
    std::atomic<int> mid_{1};
};

}  // namespace alife
