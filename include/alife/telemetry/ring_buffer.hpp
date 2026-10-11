#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace alife::telemetry {

// Fixed-capacity history. at(0) is the oldest retained sample.
template <class T>
class RingBuffer {
   public:
    explicit RingBuffer(std::size_t capacity = 512) : data_(capacity) {}

    void push(T v) {
        data_[head_] = v;
        head_ = (head_ + 1) % data_.size();
        if (size_ < data_.size()) ++size_;
        ++pushed_;
    }

    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return data_.size(); }
    std::uint64_t totalPushed() const noexcept { return pushed_; }

    T at(std::size_t i) const {
        const std::size_t start = (head_ + data_.size() - size_) % data_.size();
        return data_[(start + i) % data_.size()];
    }
    T back() const { return at(size_ - 1); }

   private:
    std::vector<T> data_;
    std::size_t head_ = 0, size_ = 0;
    std::uint64_t pushed_ = 0;
};

}  // namespace alife::telemetry
