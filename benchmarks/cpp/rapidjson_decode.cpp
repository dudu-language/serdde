#include "rapidjson_codec.hpp"

#include <rapidjson/memorystream.h>
#include <rapidjson/reader.h>

#include <limits>
#include <string_view>

namespace serdde_benchmark {
namespace {

using rapidjson::SizeType;

template <typename Derived>
struct DirectHandler
    : rapidjson::BaseReaderHandler<rapidjson::UTF8<>, Derived> {
  Derived &self() { return static_cast<Derived &>(*this); }

  bool Null() { return false; }
  bool Bool(bool value) { return self().boolean(value); }
  bool Int(int value) { return self().signed_integer(value); }
  bool Uint(unsigned value) { return self().unsigned_integer(value); }
  bool Int64(std::int64_t value) { return self().signed_integer(value); }
  bool Uint64(std::uint64_t value) { return self().unsigned_integer(value); }
  bool Double(double) { return false; }
  bool RawNumber(const char *, SizeType, bool) { return false; }
  bool String(const char *value, SizeType size, bool) {
    return self().string(std::string_view(value, size));
  }
  bool Key(const char *value, SizeType size, bool) {
    return self().key(std::string_view(value, size));
  }
  bool StartObject() { return self().start_object(); }
  bool EndObject(SizeType count) { return self().end_object(count); }
  bool StartArray() { return self().start_array(); }
  bool EndArray(SizeType count) { return self().end_array(count); }
};

template <typename T> bool assign_signed(std::int64_t value, T &output) {
  if (value < std::numeric_limits<T>::min() ||
      value > std::numeric_limits<T>::max()) {
    return false;
  }
  output = static_cast<T>(value);
  return true;
}

struct SmallHandler : DirectHandler<SmallHandler> {
  explicit SmallHandler(SmallRecord &value) : output(value) {
    output.scores.clear();
    output.metadata.clear();
  }

  bool boolean(bool value) {
    if (context != Context::root || current != "active")
      return false;
    output.active = value;
    seen |= 4;
    return true;
  }
  bool signed_integer(std::int64_t value) {
    if (context == Context::scores) {
      std::int32_t score = 0;
      if (!assign_signed(value, score))
        return false;
      output.scores.push_back(score);
      return true;
    }
    return value >= 0 && unsigned_integer(static_cast<std::uint64_t>(value));
  }
  bool unsigned_integer(std::uint64_t value) {
    if (context == Context::scores) {
      if (value >
          static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
        return false;
      output.scores.push_back(static_cast<std::int32_t>(value));
      return true;
    }
    if (context != Context::root || current != "id")
      return false;
    output.id = value;
    seen |= 1;
    return true;
  }
  bool string(std::string_view value) {
    if (context == Context::root && current == "name") {
      output.name.assign(value);
      seen |= 2;
      return true;
    }
    if (context == Context::metadata && !current.empty()) {
      output.metadata.emplace(current, value);
      current.clear();
      return true;
    }
    return false;
  }
  bool key(std::string_view value) {
    current.assign(value);
    return context == Context::metadata || value == "id" || value == "name" ||
           value == "active" || value == "scores" || value == "metadata";
  }
  bool start_object() {
    if (context == Context::none) {
      context = Context::root;
      return true;
    }
    if (context == Context::root && current == "metadata") {
      context = Context::metadata;
      seen |= 16;
      current.clear();
      return true;
    }
    return false;
  }
  bool end_object(SizeType) {
    if (context == Context::metadata) {
      context = Context::root;
      return true;
    }
    if (context == Context::root && seen == 31) {
      context = Context::done;
      return true;
    }
    return false;
  }
  bool start_array() {
    if (context != Context::root || current != "scores")
      return false;
    context = Context::scores;
    seen |= 8;
    return true;
  }
  bool end_array(SizeType) {
    if (context != Context::scores)
      return false;
    context = Context::root;
    return true;
  }
  bool complete() const { return context == Context::done; }

private:
  enum class Context { none, root, scores, metadata, done };
  SmallRecord &output;
  Context context{Context::none};
  std::string current;
  unsigned seen{};
};

struct NestedHandler : DirectHandler<NestedHandler> {
  explicit NestedHandler(NestedRecord &value) : output(value) {}

  bool boolean(bool) { return false; }
  bool signed_integer(std::int64_t value) {
    if (depth != 5 || current != "value")
      return false;
    output.child.child.child.child.value = value;
    seen |= 1;
    return true;
  }
  bool unsigned_integer(std::uint64_t value) {
    if (value >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
      return false;
    return signed_integer(static_cast<std::int64_t>(value));
  }
  bool string(std::string_view value) {
    if (depth != 5 || current != "label")
      return false;
    output.child.child.child.child.label.assign(value);
    seen |= 2;
    return true;
  }
  bool key(std::string_view value) {
    current.assign(value);
    return depth < 5 ? value == "child" : value == "value" || value == "label";
  }
  bool start_object() {
    if (depth == 0 || (depth < 5 && current == "child")) {
      ++depth;
      return true;
    }
    return false;
  }
  bool end_object(SizeType) {
    if (depth == 0 || (depth == 5 && seen != 3))
      return false;
    --depth;
    if (depth == 0)
      done = true;
    return true;
  }
  bool start_array() { return false; }
  bool end_array(SizeType) { return false; }
  bool complete() const { return done; }

private:
  NestedRecord &output;
  std::string current;
  unsigned depth{};
  unsigned seen{};
  bool done{};
};

struct StringHandler : DirectHandler<StringHandler> {
  explicit StringHandler(StringRecord &value) : output(value) {}
  bool boolean(bool) { return false; }
  bool signed_integer(std::int64_t) { return false; }
  bool unsigned_integer(std::uint64_t) { return false; }
  bool string(std::string_view value) {
    if (depth != 1 || current != "payload")
      return false;
    output.payload.assign(value);
    seen = true;
    return true;
  }
  bool key(std::string_view value) {
    current.assign(value);
    return value == "payload";
  }
  bool start_object() { return depth++ == 0; }
  bool end_object(SizeType) {
    if (depth != 1 || !seen)
      return false;
    depth = 0;
    done = true;
    return true;
  }
  bool start_array() { return false; }
  bool end_array(SizeType) { return false; }
  bool complete() const { return done; }

private:
  StringRecord &output;
  std::string current;
  unsigned depth{};
  bool seen{};
  bool done{};
};

struct ArrayHandler : DirectHandler<ArrayHandler> {
  explicit ArrayHandler(ArrayRecord &value) : output(value) {
    output.values.clear();
  }
  bool boolean(bool) { return false; }
  bool signed_integer(std::int64_t value) {
    if (!in_array)
      return false;
    output.values.push_back(value);
    return true;
  }
  bool unsigned_integer(std::uint64_t value) {
    if (value >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
      return false;
    return signed_integer(static_cast<std::int64_t>(value));
  }
  bool string(std::string_view) { return false; }
  bool key(std::string_view value) {
    current.assign(value);
    return value == "values";
  }
  bool start_object() { return !root && (root = true); }
  bool end_object(SizeType) {
    if (!root || in_array || !seen)
      return false;
    done = true;
    return true;
  }
  bool start_array() {
    if (!root || current != "values" || in_array)
      return false;
    in_array = true;
    seen = true;
    return true;
  }
  bool end_array(SizeType) {
    if (!in_array)
      return false;
    in_array = false;
    return true;
  }
  bool complete() const { return done; }

private:
  ArrayRecord &output;
  std::string current;
  bool root{};
  bool in_array{};
  bool seen{};
  bool done{};
};

struct MapHandler : DirectHandler<MapHandler> {
  explicit MapHandler(MapRecord &value) : output(value) {
    output.values.clear();
  }
  bool boolean(bool) { return false; }
  bool signed_integer(std::int64_t value) {
    if (depth != 2 || current.empty())
      return false;
    output.values.emplace(current, value);
    current.clear();
    return true;
  }
  bool unsigned_integer(std::uint64_t value) {
    if (value >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
      return false;
    return signed_integer(static_cast<std::int64_t>(value));
  }
  bool string(std::string_view) { return false; }
  bool key(std::string_view value) {
    current.assign(value);
    return depth == 2 || value == "values";
  }
  bool start_object() {
    if (depth == 0 || (depth == 1 && current == "values")) {
      ++depth;
      return true;
    }
    return false;
  }
  bool end_object(SizeType) {
    if (depth == 2) {
      depth = 1;
      seen = true;
      return true;
    }
    if (depth == 1 && seen) {
      depth = 0;
      done = true;
      return true;
    }
    return false;
  }
  bool start_array() { return false; }
  bool end_array(SizeType) { return false; }
  bool complete() const { return done; }

private:
  MapRecord &output;
  std::string current;
  unsigned depth{};
  bool seen{};
  bool done{};
};

struct EventHandler : DirectHandler<EventHandler> {
  explicit EventHandler(EventRecord &value) : output(value) {}
  bool boolean(bool) { return false; }
  bool signed_integer(std::int64_t value) {
    if (depth != 3)
      return value >= 0 && unsigned_integer(static_cast<std::uint64_t>(value));
    if (kind == "KeyDown" && current == "key") {
      std::int32_t key_value = 0;
      if (!assign_signed(value, key_value))
        return false;
      output.event = KeyDown{key_value};
      payload_seen = true;
      return true;
    }
    if (kind == "MouseMove" && current == "x") {
      return assign_signed(value, move.x) && (move_seen |= 1, true);
    }
    if (kind == "MouseMove" && current == "y") {
      return assign_signed(value, move.y) && (move_seen |= 2, true);
    }
    return false;
  }
  bool unsigned_integer(std::uint64_t value) {
    if (depth == 1 && current == "sequence") {
      output.sequence = value;
      sequence_seen = true;
      return true;
    }
    if (value >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
      return false;
    return signed_integer(static_cast<std::int64_t>(value));
  }
  bool string(std::string_view value) {
    if (depth != 1 || current != "event" || value != "Quit")
      return false;
    output.event = Quit{};
    payload_seen = true;
    return true;
  }
  bool key(std::string_view value) {
    current.assign(value);
    if (depth == 1)
      return value == "sequence" || value == "event";
    if (depth == 2) {
      kind.assign(value);
      return value == "KeyDown" || value == "MouseMove";
    }
    return (kind == "KeyDown" && value == "key") ||
           (kind == "MouseMove" && (value == "x" || value == "y"));
  }
  bool start_object() {
    if (depth == 0 || (depth == 1 && current == "event") ||
        (depth == 2 && (kind == "KeyDown" || kind == "MouseMove"))) {
      ++depth;
      return true;
    }
    return false;
  }
  bool end_object(SizeType) {
    if (depth == 3) {
      if (kind == "MouseMove" && move_seen == 3) {
        output.event = move;
        payload_seen = true;
      }
      if (!payload_seen)
        return false;
      --depth;
      return true;
    }
    if (depth == 2) {
      --depth;
      return true;
    }
    if (depth == 1 && sequence_seen && payload_seen) {
      depth = 0;
      done = true;
      return true;
    }
    return false;
  }
  bool start_array() { return false; }
  bool end_array(SizeType) { return false; }
  bool complete() const { return done; }

private:
  EventRecord &output;
  MouseMove move{};
  std::string current;
  std::string kind;
  unsigned depth{};
  unsigned move_seen{};
  bool sequence_seen{};
  bool payload_seen{};
  bool done{};
};

template <typename Handler, typename T>
bool decode(const std::string &source, T &output) {
  rapidjson::MemoryStream stream(source.data(), source.size());
  rapidjson::Reader reader;
  Handler handler(output);
  constexpr unsigned flags = rapidjson::kParseValidateEncodingFlag;
  return reader.Parse<flags>(stream, handler) && handler.complete();
}

} // namespace

bool rapidjson_decode(const std::string &source, SmallRecord &output) {
  return decode<SmallHandler>(source, output);
}
bool rapidjson_decode(const std::string &source, NestedRecord &output) {
  return decode<NestedHandler>(source, output);
}
bool rapidjson_decode(const std::string &source, StringRecord &output) {
  return decode<StringHandler>(source, output);
}
bool rapidjson_decode(const std::string &source, ArrayRecord &output) {
  return decode<ArrayHandler>(source, output);
}
bool rapidjson_decode(const std::string &source, MapRecord &output) {
  return decode<MapHandler>(source, output);
}
bool rapidjson_decode(const std::string &source, EventRecord &output) {
  return decode<EventHandler>(source, output);
}

} // namespace serdde_benchmark
