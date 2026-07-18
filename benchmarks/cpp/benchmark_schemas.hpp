#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace serdde_benchmark {

struct SmallRecord {
  std::uint64_t id{};
  std::string name;
  bool active{};
  std::vector<std::int32_t> scores;
  std::map<std::string, std::string> metadata;
};

struct NestedLeaf {
  std::int64_t value{};
  std::string label;
};

struct NestedLevel4 {
  NestedLeaf child;
};

struct NestedLevel3 {
  NestedLevel4 child;
};

struct NestedLevel2 {
  NestedLevel3 child;
};

struct NestedRecord {
  NestedLevel2 child;
};

struct StringRecord {
  std::string payload;
};

struct ArrayRecord {
  std::vector<std::int64_t> values;
};

struct MapRecord {
  std::map<std::string, std::int64_t> values;
};

struct Quit {};

struct KeyDown {
  std::int32_t key{};
};

struct MouseMove {
  std::int32_t x{};
  std::int32_t y{};
};

using BenchEvent = std::variant<Quit, KeyDown, MouseMove>;

struct EventRecord {
  std::uint64_t sequence{};
  BenchEvent event;
};

inline SmallRecord small_record() {
  return {7,
          "Ada Lovelace",
          true,
          {2, 3, 5, 7, 11, 13, 17, 19},
          {{"format", "wire"}, {"language", "dudu"}}};
}

inline NestedRecord nested_record() {
  return {{{{{-123456789, "deep leaf"}}}}};
}

inline StringRecord string_record() {
  StringRecord record;
  constexpr std::string_view chunk = "Dudu direct wire payload. ";
  record.payload.reserve(chunk.size() * 2622);
  for (std::size_t index = 0; index < 2622; ++index) {
    record.payload += chunk;
  }
  return record;
}

inline ArrayRecord array_record() {
  ArrayRecord record;
  record.values.reserve(4096);
  for (std::int64_t index = 0; index < 4096; ++index) {
    record.values.push_back((index * 17) - 2048);
  }
  return record;
}

inline MapRecord map_record() {
  MapRecord record;
  for (std::int64_t index = 0; index < 512; ++index) {
    record.values["key_" + std::to_string(index)] = (index * 31) - 7;
  }
  return record;
}

inline EventRecord event_record() { return {99, MouseMove{320, -180}}; }

inline std::uint64_t checksum(const SmallRecord &value) {
  return value.id + value.name.size() + value.scores.size() +
         value.metadata.size() + static_cast<std::uint64_t>(value.active);
}

inline std::uint64_t checksum(const NestedRecord &value) {
  return static_cast<std::uint64_t>(value.child.child.child.child.value) +
         value.child.child.child.child.label.size();
}

inline std::uint64_t checksum(const StringRecord &value) {
  return value.payload.size();
}

inline std::uint64_t checksum(const ArrayRecord &value) {
  return value.values.size() + static_cast<std::uint64_t>(value.values.back());
}

inline std::uint64_t checksum(const MapRecord &value) {
  return value.values.size() +
         static_cast<std::uint64_t>(value.values.at("key_511"));
}

inline std::uint64_t checksum(const EventRecord &value) {
  return value.sequence +
         std::visit(
             [](const auto &event) -> std::uint64_t {
               using T = std::decay_t<decltype(event)>;
               if constexpr (std::is_same_v<T, KeyDown>) {
                 return static_cast<std::uint64_t>(event.key);
               } else if constexpr (std::is_same_v<T, MouseMove>) {
                 return static_cast<std::uint64_t>(event.x + event.y);
               }
               return 0;
             },
             value.event);
}

} // namespace serdde_benchmark
