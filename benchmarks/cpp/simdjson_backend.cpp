#include "benchmark_fixture_json.hpp"
#include "benchmark_runtime.hpp"
#include "simdjson_codec.hpp"

namespace serdde_benchmark {

struct SimdjsonBackend {
  static const char *name() { return "simdjson-ondemand"; }
  static bool supports(const std::string &format) { return format == "json"; }

  template <typename T>
  static bool encode(const T &value, const std::string &, std::string &output) {
    return benchmark_json(value, output);
  }

  template <typename T>
  static bool decode(const std::string &source, const std::string &,
                     T &output) {
    return simdjson_decode(source, output);
  }
};

} // namespace serdde_benchmark

int main() {
  return serdde_benchmark::benchmark_main<serdde_benchmark::SimdjsonBackend>();
}
