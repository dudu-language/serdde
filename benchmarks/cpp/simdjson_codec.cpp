#include "simdjson_codec.hpp"

#include <simdjson.h>

#include <limits>
#include <string_view>

namespace serdde_benchmark {
namespace {

namespace od = simdjson::ondemand;

bool read_text(od::value &value, std::string &output) {
  std::string_view text;
  if (value.get_string().get(text))
    return false;
  output.assign(text);
  return true;
}

template <typename T> bool read_signed(od::value &value, T &output) {
  std::int64_t number = 0;
  if (value.get_int64().get(number) || number < std::numeric_limits<T>::min() ||
      number > std::numeric_limits<T>::max()) {
    return false;
  }
  output = static_cast<T>(number);
  return true;
}

bool read_object(od::object object, NestedLeaf &output);
bool read_object(od::object object, NestedLevel4 &output);
bool read_object(od::object object, NestedLevel3 &output);
bool read_object(od::object object, NestedLevel2 &output);

template <typename T> bool read_nested_child(od::object object, T &output) {
  bool seen = false;
  for (auto field : object) {
    std::string_view name;
    if (field.unescaped_key().get(name) || name != "child" || seen) {
      return false;
    }
    od::object child;
    if (field.value().get_object().get(child) ||
        !read_object(child, output.child)) {
      return false;
    }
    seen = true;
  }
  return seen;
}

bool read_object(od::object object, SmallRecord &output) {
  output.scores.clear();
  output.metadata.clear();
  unsigned seen = 0;
  for (auto field : object) {
    std::string_view name;
    if (field.unescaped_key().get(name))
      return false;
    od::value value;
    if (field.value().get(value))
      return false;
    if (name == "id") {
      if ((seen & 1) || value.get_uint64().get(output.id))
        return false;
      seen |= 1;
    } else if (name == "name") {
      if ((seen & 2) || !read_text(value, output.name))
        return false;
      seen |= 2;
    } else if (name == "active") {
      if ((seen & 4) || value.get_bool().get(output.active))
        return false;
      seen |= 4;
    } else if (name == "scores") {
      if (seen & 8)
        return false;
      od::array scores;
      if (value.get_array().get(scores))
        return false;
      for (auto item : scores) {
        std::int64_t number = 0;
        if (item.get_int64().get(number) ||
            number < std::numeric_limits<std::int32_t>::min() ||
            number > std::numeric_limits<std::int32_t>::max())
          return false;
        output.scores.push_back(static_cast<std::int32_t>(number));
      }
      seen |= 8;
    } else if (name == "metadata") {
      if (seen & 16)
        return false;
      od::object metadata;
      if (value.get_object().get(metadata))
        return false;
      for (auto item : metadata) {
        std::string key;
        std::string item_value;
        od::value item_wire;
        if (item.unescaped_key().get(key) || item.value().get(item_wire) ||
            !read_text(item_wire, item_value))
          return false;
        output.metadata.emplace(std::move(key), std::move(item_value));
      }
      seen |= 16;
    } else {
      return false;
    }
  }
  return seen == 31;
}

bool read_object(od::object object, NestedLeaf &output) {
  unsigned seen = 0;
  for (auto field : object) {
    std::string_view name;
    if (field.unescaped_key().get(name))
      return false;
    if (name == "value") {
      od::value value;
      if ((seen & 1) || field.value().get(value) ||
          !read_signed(value, output.value))
        return false;
      seen |= 1;
    } else if (name == "label") {
      od::value value;
      if ((seen & 2) || field.value().get(value) ||
          !read_text(value, output.label))
        return false;
      seen |= 2;
    } else {
      return false;
    }
  }
  return seen == 3;
}

bool read_object(od::object object, NestedLevel4 &output) {
  return read_nested_child(object, output);
}
bool read_object(od::object object, NestedLevel3 &output) {
  return read_nested_child(object, output);
}
bool read_object(od::object object, NestedLevel2 &output) {
  return read_nested_child(object, output);
}
bool read_object(od::object object, NestedRecord &output) {
  return read_nested_child(object, output);
}

bool read_object(od::object object, StringRecord &output) {
  bool seen = false;
  for (auto field : object) {
    std::string_view name;
    od::value value;
    if (field.unescaped_key().get(name) || name != "payload" || seen ||
        field.value().get(value) || !read_text(value, output.payload)) {
      return false;
    }
    seen = true;
  }
  return seen;
}

bool read_object(od::object object, ArrayRecord &output) {
  output.values.clear();
  bool seen = false;
  for (auto field : object) {
    std::string_view name;
    if (field.unescaped_key().get(name) || name != "values" || seen) {
      return false;
    }
    od::array values;
    if (field.value().get_array().get(values))
      return false;
    for (auto item : values) {
      std::int64_t value = 0;
      if (item.get_int64().get(value))
        return false;
      output.values.push_back(value);
    }
    seen = true;
  }
  return seen;
}

bool read_object(od::object object, MapRecord &output) {
  output.values.clear();
  bool seen = false;
  for (auto field : object) {
    std::string_view name;
    if (field.unescaped_key().get(name) || name != "values" || seen) {
      return false;
    }
    od::object values;
    if (field.value().get_object().get(values))
      return false;
    for (auto item : values) {
      std::string key;
      std::int64_t value = 0;
      if (item.unescaped_key().get(key) || item.value().get_int64().get(value))
        return false;
      output.values.emplace(std::move(key), value);
    }
    seen = true;
  }
  return seen;
}

bool read_event(od::value &value, BenchEvent &output) {
  od::json_type type;
  if (value.type().get(type))
    return false;
  if (type == od::json_type::string) {
    std::string_view tag;
    if (value.get_string().get(tag) || tag != "Quit")
      return false;
    output = Quit{};
    return true;
  }
  if (type != od::json_type::object)
    return false;
  od::object envelope;
  if (value.get_object().get(envelope))
    return false;
  bool seen = false;
  for (auto field : envelope) {
    std::string_view tag;
    if (field.unescaped_key().get(tag) || seen)
      return false;
    od::object payload;
    if (field.value().get_object().get(payload))
      return false;
    if (tag == "KeyDown") {
      KeyDown event;
      bool key_seen = false;
      for (auto item : payload) {
        std::string_view name;
        od::value value;
        if (item.unescaped_key().get(name) || name != "key" || key_seen ||
            item.value().get(value) || !read_signed(value, event.key)) {
          return false;
        }
        key_seen = true;
      }
      if (!key_seen)
        return false;
      output = event;
    } else if (tag == "MouseMove") {
      MouseMove event;
      unsigned fields = 0;
      for (auto item : payload) {
        std::string_view name;
        od::value value;
        if (item.unescaped_key().get(name) || item.value().get(value)) {
          return false;
        }
        if (name == "x" && !(fields & 1) && read_signed(value, event.x))
          fields |= 1;
        else if (name == "y" && !(fields & 2) && read_signed(value, event.y))
          fields |= 2;
        else
          return false;
      }
      if (fields != 3)
        return false;
      output = event;
    } else {
      return false;
    }
    seen = true;
  }
  return seen;
}

bool read_object(od::object object, EventRecord &output) {
  unsigned seen = 0;
  for (auto field : object) {
    std::string_view name;
    if (field.unescaped_key().get(name))
      return false;
    if (name == "sequence") {
      if ((seen & 1) || field.value().get_uint64().get(output.sequence)) {
        return false;
      }
      seen |= 1;
    } else if (name == "event") {
      od::value value;
      if ((seen & 2) || field.value().get(value) ||
          !read_event(value, output.event))
        return false;
      seen |= 2;
    } else {
      return false;
    }
  }
  return seen == 3;
}

template <typename T> bool decode(const std::string &source, T &output) {
  simdjson::padded_string padded(source);
  thread_local od::parser parser;
  od::document document;
  if (parser.iterate(padded).get(document))
    return false;
  od::object object;
  if (document.get_object().get(object) || !read_object(object, output)) {
    return false;
  }
  return document.at_end();
}

} // namespace

#define SERDDE_SIMDJSON_DECODE(Type)                                           \
  bool simdjson_decode(const std::string &source, Type &output) {              \
    return decode(source, output);                                             \
  }

SERDDE_SIMDJSON_DECODE(SmallRecord)
SERDDE_SIMDJSON_DECODE(NestedRecord)
SERDDE_SIMDJSON_DECODE(StringRecord)
SERDDE_SIMDJSON_DECODE(ArrayRecord)
SERDDE_SIMDJSON_DECODE(MapRecord)
SERDDE_SIMDJSON_DECODE(EventRecord)

#undef SERDDE_SIMDJSON_DECODE

} // namespace serdde_benchmark
