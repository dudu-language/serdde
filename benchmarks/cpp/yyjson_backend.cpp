#include "benchmark_runtime.hpp"
#include "yyjson_codec.hpp"

namespace serdde_benchmark {

struct YyjsonBackend {
  static const char *name() { return "yyjson"; }
  static bool supports(const std::string &format) { return format == "json"; }

  template <typename T>
  static bool encode(const T &value, const std::string &, std::string &output) {
    return yyjson_encode(value, output);
  }

  template <typename T>
  static bool decode(const std::string &source, const std::string &,
                     T &output) {
    return yyjson_decode(source, output);
  }
};

} // namespace serdde_benchmark

int main() {
  return serdde_benchmark::benchmark_main<serdde_benchmark::YyjsonBackend>();
}
