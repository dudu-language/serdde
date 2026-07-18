#pragma once

#include "benchmark_schemas.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace serdde_benchmark {

void reset_allocations();
std::uint64_t allocation_count();
std::uint64_t allocated_bytes();

inline std::string environment(const char *name, const char *fallback) {
  const char *value = std::getenv(name);
  return value == nullptr ? fallback : value;
}

inline std::int32_t iterations() {
  const std::string value = environment("SERDDE_BENCH_ITERATIONS", "1000");
  const long parsed = std::strtol(value.c_str(), nullptr, 10);
  return parsed > 0 ? static_cast<std::int32_t>(parsed) : 1000;
}

inline std::uint64_t now_ns() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline void report(const std::string &implementation, const std::string &format,
                   const std::string &workload, const std::string &operation,
                   std::int32_t count, std::uint64_t elapsed_ns,
                   std::size_t wire_bytes, std::uint64_t allocations,
                   std::uint64_t allocation_bytes, std::uint64_t sum) {
  std::cout << "implementation=" << implementation << '\n'
            << "format=" << format << '\n'
            << "workload=" << workload << '\n'
            << "operation=" << operation << '\n'
            << "iterations=" << count << '\n'
            << "elapsed_ns=" << elapsed_ns << '\n'
            << "wire_bytes=" << wire_bytes << '\n'
            << "allocation_count=" << allocations << '\n'
            << "allocated_bytes=" << allocation_bytes << '\n'
            << "checksum=" << sum << '\n';
}

template <typename Backend, typename T>
int run_typed_case(const T &value, const std::string &workload,
                   const std::string &format, const std::string &operation,
                   std::int32_t count) {
  std::string source;
  if (!Backend::encode(value, format, source)) {
    std::cerr << "initial encode failed\n";
    return 2;
  }

  reset_allocations();
  const std::uint64_t start = now_ns();
  std::uint64_t sum = 0;
  if (operation == "decode") {
    for (std::int32_t index = 0; index < count; ++index) {
      T decoded{};
      if (!Backend::decode(source, format, decoded)) {
        std::cerr << "decode failed\n";
        return 3;
      }
      sum += checksum(decoded);
    }
  } else {
    for (std::int32_t index = 0; index < count; ++index) {
      std::string encoded;
      if (!Backend::encode(value, format, encoded)) {
        std::cerr << "encode failed\n";
        return 4;
      }
      sum += encoded.size();
    }
  }
  const std::uint64_t finish = now_ns();
  const std::uint64_t allocations = allocation_count();
  const std::uint64_t bytes = allocated_bytes();
  report(Backend::name(), format, workload, operation, count, finish - start,
         source.size(), allocations, bytes, sum);
  return 0;
}

template <typename Backend>
int run_malformed(const std::string &format, std::int32_t count) {
  const std::string source =
      format == "cbor" ? std::string("a", 1) : "{\"id\":7,\"name\":\"Ada\",]";
  reset_allocations();
  const std::uint64_t start = now_ns();
  std::uint64_t rejected = 0;
  for (std::int32_t index = 0; index < count; ++index) {
    SmallRecord decoded{};
    if (!Backend::decode(source, format, decoded)) {
      ++rejected;
    }
  }
  const std::uint64_t finish = now_ns();
  const std::uint64_t allocations = allocation_count();
  const std::uint64_t bytes = allocated_bytes();
  report(Backend::name(), format, "malformed", "decode", count, finish - start,
         source.size(), allocations, bytes, rejected);
  return rejected == static_cast<std::uint64_t>(count) ? 0 : 5;
}

template <typename Backend> int benchmark_main() {
  const std::string format = environment("SERDDE_BENCH_FORMAT", "json");
  const std::string workload = environment("SERDDE_BENCH_WORKLOAD", "small");
  const std::string operation = environment("SERDDE_BENCH_OPERATION", "encode");
  const std::int32_t count = iterations();
  if (!Backend::supports(format)) {
    std::cerr << Backend::name() << " does not support " << format << '\n';
    return 6;
  }
  if (workload == "small") {
    return run_typed_case<Backend>(small_record(), workload, format, operation,
                                   count);
  }
  if (workload == "nested") {
    return run_typed_case<Backend>(nested_record(), workload, format, operation,
                                   count);
  }
  if (workload == "large_string") {
    return run_typed_case<Backend>(string_record(), workload, format, operation,
                                   count);
  }
  if (workload == "large_array") {
    return run_typed_case<Backend>(array_record(), workload, format, operation,
                                   count);
  }
  if (workload == "map") {
    return run_typed_case<Backend>(map_record(), workload, format, operation,
                                   count);
  }
  if (workload == "enum") {
    return run_typed_case<Backend>(event_record(), workload, format, operation,
                                   count);
  }
  if (workload == "malformed") {
    return run_malformed<Backend>(format, count);
  }
  std::cerr << "unknown workload: " << workload << '\n';
  return 7;
}

} // namespace serdde_benchmark
