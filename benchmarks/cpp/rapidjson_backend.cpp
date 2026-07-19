#include "benchmark_runtime.hpp"
#include "rapidjson_codec.hpp"

namespace serdde_benchmark {

struct RapidjsonBackend {
  static const char *name() { return "rapidjson-sax"; }
  static bool supports(const std::string &format) { return format == "json"; }

  template <typename T>
  static bool encode(const T &value, const std::string &, std::string &output) {
    return rapidjson_encode(value, output);
  }

  template <typename T>
  static bool decode(const std::string &source, const std::string &,
                     T &output) {
    return rapidjson_decode(source, output);
  }
};

} // namespace serdde_benchmark

int main() {
  return serdde_benchmark::benchmark_main<serdde_benchmark::RapidjsonBackend>();
}
