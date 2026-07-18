#include "benchmark_native.hpp"

#include <atomic>
#include <chrono>
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

std::string environment(const std::string &name,
                        const std::string &fallback) {
  const char *value = std::getenv(name.c_str());
  return value == nullptr ? fallback : std::string(value);
}

std::int32_t environment_i32(const std::string &name,
                             std::int32_t fallback) {
  const char *value = std::getenv(name.c_str());
  if (value == nullptr) {
    return fallback;
  }
  char *end = nullptr;
  const long parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0' || parsed < 1 ||
      parsed > std::numeric_limits<std::int32_t>::max()) {
    return fallback;
  }
  return static_cast<std::int32_t>(parsed);
}

std::string repeat_text(const std::string &chunk, std::size_t count) {
  std::string result;
  result.reserve(chunk.size() * count);
  for (std::size_t index = 0; index < count; ++index) {
    result += chunk;
  }
  return result;
}

std::uint64_t now_ns() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

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

