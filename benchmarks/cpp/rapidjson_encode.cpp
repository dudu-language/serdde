#include "rapidjson_codec.hpp"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <type_traits>

namespace serdde_benchmark {
namespace {

using Buffer = rapidjson::StringBuffer;
using Writer = rapidjson::Writer<Buffer>;

void write(Writer &writer, const NestedLeaf &value);
void write(Writer &writer, const NestedLevel4 &value);
void write(Writer &writer, const NestedLevel3 &value);
void write(Writer &writer, const NestedLevel2 &value);

void key(Writer &writer, const char *value) { writer.Key(value); }

void text(Writer &writer, const std::string &value) {
  writer.String(value.data(), static_cast<rapidjson::SizeType>(value.size()));
}

void write(Writer &writer, const SmallRecord &value) {
  writer.StartObject();
  key(writer, "id");
  writer.Uint64(value.id);
  key(writer, "name");
  text(writer, value.name);
  key(writer, "active");
  writer.Bool(value.active);
  key(writer, "scores");
  writer.StartArray();
  for (const std::int32_t score : value.scores)
    writer.Int(score);
  writer.EndArray();
  key(writer, "metadata");
  writer.StartObject();
  for (const auto &[name, item] : value.metadata) {
    text(writer, name);
    text(writer, item);
  }
  writer.EndObject();
  writer.EndObject();
}

void write(Writer &writer, const NestedLeaf &value) {
  writer.StartObject();
  key(writer, "value");
  writer.Int64(value.value);
  key(writer, "label");
  text(writer, value.label);
  writer.EndObject();
}

template <typename T> void write_child(Writer &writer, const T &value) {
  writer.StartObject();
  key(writer, "child");
  write(writer, value.child);
  writer.EndObject();
}

void write(Writer &writer, const NestedLevel4 &value) {
  write_child(writer, value);
}

void write(Writer &writer, const NestedLevel3 &value) {
  write_child(writer, value);
}

void write(Writer &writer, const NestedLevel2 &value) {
  write_child(writer, value);
}

void write(Writer &writer, const NestedRecord &value) {
  write_child(writer, value);
}

void write(Writer &writer, const StringRecord &value) {
  writer.StartObject();
  key(writer, "payload");
  text(writer, value.payload);
  writer.EndObject();
}

void write(Writer &writer, const ArrayRecord &value) {
  writer.StartObject();
  key(writer, "values");
  writer.StartArray();
  for (const std::int64_t item : value.values)
    writer.Int64(item);
  writer.EndArray();
  writer.EndObject();
}

void write(Writer &writer, const MapRecord &value) {
  writer.StartObject();
  key(writer, "values");
  writer.StartObject();
  for (const auto &[name, item] : value.values) {
    text(writer, name);
    writer.Int64(item);
  }
  writer.EndObject();
  writer.EndObject();
}

void write(Writer &writer, const EventRecord &value) {
  writer.StartObject();
  key(writer, "sequence");
  writer.Uint64(value.sequence);
  key(writer, "event");
  std::visit(
      [&](const auto &event) {
        using T = std::decay_t<decltype(event)>;
        if constexpr (std::is_same_v<T, Quit>) {
          writer.String("Quit");
        } else {
          writer.StartObject();
          if constexpr (std::is_same_v<T, KeyDown>) {
            key(writer, "KeyDown");
            writer.StartObject();
            key(writer, "key");
            writer.Int(event.key);
          } else {
            key(writer, "MouseMove");
            writer.StartObject();
            key(writer, "x");
            writer.Int(event.x);
            key(writer, "y");
            writer.Int(event.y);
          }
          writer.EndObject();
          writer.EndObject();
        }
      },
      value.event);
  writer.EndObject();
}

template <typename T> bool encode(const T &value, std::string &output) {
  Buffer buffer;
  Writer writer(buffer);
  write(writer, value);
  if (!writer.IsComplete())
    return false;
  output.assign(buffer.GetString(), buffer.GetSize());
  return true;
}

} // namespace

#define SERDDE_RAPIDJSON_ENCODE(Type)                                          \
  bool rapidjson_encode(const Type &value, std::string &output) {              \
    return encode(value, output);                                              \
  }

SERDDE_RAPIDJSON_ENCODE(SmallRecord)
SERDDE_RAPIDJSON_ENCODE(NestedRecord)
SERDDE_RAPIDJSON_ENCODE(StringRecord)
SERDDE_RAPIDJSON_ENCODE(ArrayRecord)
SERDDE_RAPIDJSON_ENCODE(MapRecord)
SERDDE_RAPIDJSON_ENCODE(EventRecord)

#undef SERDDE_RAPIDJSON_ENCODE

} // namespace serdde_benchmark
