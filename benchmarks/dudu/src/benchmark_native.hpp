#pragma once

#include <cstdint>
#include <string>

namespace serdde_benchmark {

std::string environment(const std::string &name,
                        const std::string &fallback);
std::int32_t environment_i32(const std::string &name,
                             std::int32_t fallback);
std::string repeat_text(const std::string &chunk, std::size_t count);
std::uint64_t now_ns();
void reset_allocations();
std::uint64_t allocation_count();
std::uint64_t allocated_bytes();

template <typename T> inline void consume(const T &value) {
#if defined(__GNUC__) || defined(__clang__)
  asm volatile("" : : "g"(&value) : "memory");
#else
  volatile const void *pointer = &value;
  (void)pointer;
#endif
}

} // namespace serdde_benchmark
