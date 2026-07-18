#include "benchmark_runtime.hpp"

#include <nlohmann/json.hpp>

namespace serdde_benchmark {

using nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SmallRecord, id, name, active, scores,
                                   metadata)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NestedLeaf, value, label)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NestedLevel4, child)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NestedLevel3, child)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NestedLevel2, child)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NestedRecord, child)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StringRecord, payload)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ArrayRecord, values)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapRecord, values)

void to_json(json &output, const EventRecord &record) {
  json event;
  std::visit(
      [&](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Quit>) {
          event = "Quit";
        } else if constexpr (std::is_same_v<T, KeyDown>) {
          event = {{"KeyDown", {{"key", value.key}}}};
        } else {
          event = {{"MouseMove", {{"x", value.x}, {"y", value.y}}}};
        }
      },
      record.event);
  output = {{"sequence", record.sequence}, {"event", std::move(event)}};
}

void from_json(const json &input, EventRecord &record) {
  input.at("sequence").get_to(record.sequence);
  const json &event = input.at("event");
  if (event.is_string() && event.get<std::string>() == "Quit") {
    record.event = Quit{};
  } else if (event.contains("KeyDown")) {
    record.event = KeyDown{event.at("KeyDown").at("key").get<std::int32_t>()};
  } else {
    const json &move = event.at("MouseMove");
    record.event = MouseMove{move.at("x").get<std::int32_t>(),
                             move.at("y").get<std::int32_t>()};
  }
}

struct NlohmannBackend {
  static const char *name() { return "nlohmann"; }
  static bool supports(const std::string &format) {
    return format == "json" || format == "cbor";
  }

  template <typename T>
  static bool encode(const T &value, const std::string &format,
                     std::string &output) {
    try {
      const json tree = value;
      if (format == "cbor") {
        const std::vector<std::uint8_t> bytes = json::to_cbor(tree);
        output.assign(reinterpret_cast<const char *>(bytes.data()),
                      bytes.size());
      } else {
        output = tree.dump();
      }
      return true;
    } catch (...) {
      return false;
    }
  }

  template <typename T>
  static bool decode(const std::string &source, const std::string &format,
                     T &output) {
    try {
      json tree;
      if (format == "cbor") {
        tree = json::from_cbor(source.begin(), source.end(), true, true);
      } else {
        tree = json::parse(source);
      }
      output = tree.template get<T>();
      return true;
    } catch (...) {
      return false;
    }
  }
};

} // namespace serdde_benchmark

int main() {
  return serdde_benchmark::benchmark_main<serdde_benchmark::NlohmannBackend>();
}
