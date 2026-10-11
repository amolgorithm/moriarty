#pragma once
#include <cstddef>
#include <new>
#include <vector>

namespace alife {

template <class T, std::size_t Align = 64>
struct AlignedAlloc {
    using value_type = T;
    template <class U>
    struct rebind {
        using other = AlignedAlloc<U, Align>;
    };

    AlignedAlloc() noexcept = default;
    template <class U>
    AlignedAlloc(const AlignedAlloc<U, Align>&) noexcept {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t{Align}));
    }
    void deallocate(T* p, std::size_t) noexcept { ::operator delete(p, std::align_val_t{Align}); }

    template <class U>
    bool operator==(const AlignedAlloc<U, Align>&) const noexcept { return true; }
};

inline constexpr std::size_t kCacheLine = 64;
template <class T>
using avec = std::vector<T, AlignedAlloc<T, kCacheLine>>;

}  // namespace alife
