#include "alife/concurrency/thread_pool.hpp"

#include <thread>

namespace alife {

ThreadPool::ThreadPool(std::size_t threads)
    : threads_(threads ? threads : std::max<std::size_t>(1, std::thread::hardware_concurrency())) {}

}  // namespace alife
