#include "benchmark_runtime.hpp"

#include <atomic>
#include <cstdlib>
#include <limits>
#include <new>

extern "C" void *__real_malloc(std::size_t);
extern "C" void *__real_calloc(std::size_t, std::size_t);
extern "C" void *__real_realloc(void *, std::size_t);
extern "C" void __real_free(void *);

namespace {

std::atomic<std::uint64_t> allocations{0};
std::atomic<std::uint64_t> bytes{0};

void record(std::size_t size) {
  allocations.fetch_add(1, std::memory_order_relaxed);
  bytes.fetch_add(static_cast<std::uint64_t>(size), std::memory_order_relaxed);
}

} // namespace

extern "C" void *__wrap_malloc(std::size_t size) {
  record(size);
  return __real_malloc(size);
}

extern "C" void *__wrap_calloc(std::size_t count, std::size_t size) {
  const std::size_t requested =
      size != 0 && count > std::numeric_limits<std::size_t>::max() / size
          ? std::numeric_limits<std::size_t>::max()
          : count * size;
  record(requested);
  return __real_calloc(count, size);
}

extern "C" void *__wrap_realloc(void *pointer, std::size_t size) {
  record(size);
  return __real_realloc(pointer, size);
}

extern "C" void __wrap_free(void *pointer) { __real_free(pointer); }

void *operator new(std::size_t size) {
  record(size);
  if (void *pointer = __real_malloc(size)) {
    return pointer;
  }
  throw std::bad_alloc();
}

void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *pointer) noexcept { __real_free(pointer); }
void operator delete[](void *pointer) noexcept { __real_free(pointer); }
void operator delete(void *pointer, std::size_t) noexcept {
  __real_free(pointer);
}
void operator delete[](void *pointer, std::size_t) noexcept {
  __real_free(pointer);
}

namespace serdde_benchmark {

void reset_allocations() {
  allocations.store(0, std::memory_order_relaxed);
  bytes.store(0, std::memory_order_relaxed);
}

std::uint64_t allocation_count() {
  return allocations.load(std::memory_order_relaxed);
}

std::uint64_t allocated_bytes() {
  return bytes.load(std::memory_order_relaxed);
}

} // namespace serdde_benchmark
