#include "benchmark_fixture_json.hpp"

#include <charconv>
#include <cstdio>
#include <string_view>
#include <type_traits>

namespace serdde_benchmark {
namespace {

void append_string(std::string &output, std::string_view value) {
  output.push_back('"');
  for (const unsigned char byte : value) {
    switch (byte) {
    case '"':
      output += "\\\"";
      break;
    case '\\':
      output += "\\\\";
      break;
    case '\b':
      output += "\\b";
      break;
    case '\f':
      output += "\\f";
      break;
    case '\n':
      output += "\\n";
      break;
    case '\r':
      output += "\\r";
      break;
    case '\t':
      output += "\\t";
      break;
    default:
      if (byte < 0x20) {
        char escaped[7]{};
        std::snprintf(escaped, sizeof(escaped), "\\u%04x", byte);
        output += escaped;
      } else {
        output.push_back(static_cast<char>(byte));
      }
    }
  }
  output.push_back('"');
}

template <typename T> void append_integer(std::string &output, T value) {
  char buffer[32]{};
  const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  output.append(buffer, result.ptr);
}

void append_leaf(std::string &output, const NestedLeaf &value) {
  output += "{\"value\":";
  append_integer(output, value.value);
  output += ",\"label\":";
  append_string(output, value.label);
  output.push_back('}');
}

template <typename T, typename Next>
void append_child(std::string &output, const T &value, Next next) {
  output += "{\"child\":";
  next(output, value.child);
  output.push_back('}');
}

void append_level4(std::string &output, const NestedLevel4 &value) {
  append_child(output, value, append_leaf);
}

void append_level3(std::string &output, const NestedLevel3 &value) {
  append_child(output, value, append_level4);
}

void append_level2(std::string &output, const NestedLevel2 &value) {
  append_child(output, value, append_level3);
}

} // namespace

bool benchmark_json(const SmallRecord &value, std::string &output) {
  output.clear();
  output += "{\"id\":";
  append_integer(output, value.id);
  output += ",\"name\":";
  append_string(output, value.name);
  output += ",\"active\":";
  output += value.active ? "true" : "false";
  output += ",\"scores\":[";
  for (std::size_t index = 0; index < value.scores.size(); ++index) {
    if (index != 0)
      output.push_back(',');
    append_integer(output, value.scores[index]);
  }
  output += "],\"metadata\":{";
  bool first = true;
  for (const auto &[key, item] : value.metadata) {
    if (!first)
      output.push_back(',');
    first = false;
    append_string(output, key);
    output.push_back(':');
    append_string(output, item);
  }
  output += "}}";
  return true;
}

bool benchmark_json(const NestedRecord &value, std::string &output) {
  output.clear();
  append_child(output, value, append_level2);
  return true;
}

bool benchmark_json(const StringRecord &value, std::string &output) {
  output = "{\"payload\":";
  append_string(output, value.payload);
  output.push_back('}');
  return true;
}

bool benchmark_json(const ArrayRecord &value, std::string &output) {
  output = "{\"values\":[";
  for (std::size_t index = 0; index < value.values.size(); ++index) {
    if (index != 0)
      output.push_back(',');
    append_integer(output, value.values[index]);
  }
  output += "]}";
  return true;
}

bool benchmark_json(const MapRecord &value, std::string &output) {
  output = "{\"values\":{";
  bool first = true;
  for (const auto &[key, item] : value.values) {
    if (!first)
      output.push_back(',');
    first = false;
    append_string(output, key);
    output.push_back(':');
    append_integer(output, item);
  }
  output += "}}";
  return true;
}

bool benchmark_json(const EventRecord &value, std::string &output) {
  output = "{\"sequence\":";
  append_integer(output, value.sequence);
  output += ",\"event\":";
  std::visit(
      [&](const auto &event) {
        using T = std::decay_t<decltype(event)>;
        if constexpr (std::is_same_v<T, Quit>) {
          output += "\"Quit\"";
        } else if constexpr (std::is_same_v<T, KeyDown>) {
          output += "{\"KeyDown\":{\"key\":";
          append_integer(output, event.key);
          output += "}}";
        } else {
          output += "{\"MouseMove\":{\"x\":";
          append_integer(output, event.x);
          output += ",\"y\":";
          append_integer(output, event.y);
          output += "}}";
        }
      },
      value.event);
  output.push_back('}');
  return true;
}

} // namespace serdde_benchmark
