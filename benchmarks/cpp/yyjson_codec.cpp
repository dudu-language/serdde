#include "yyjson_codec.hpp"

#include <yyjson.h>

#include <cstdlib>
#include <limits>
#include <type_traits>

namespace serdde_benchmark {
namespace {

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLeaf &value);
yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLevel4 &value);
yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLevel3 &value);
yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLevel2 &value);

bool add(yyjson_mut_doc *doc, yyjson_mut_val *object, const char *name,
         yyjson_mut_val *value) {
  return yyjson_mut_obj_add_val(doc, object, name, value);
}

yyjson_mut_val *text(yyjson_mut_doc *doc, const std::string &value) {
  return yyjson_mut_strncpy(doc, value.data(), value.size());
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const SmallRecord &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  yyjson_mut_val *scores = yyjson_mut_arr(doc);
  yyjson_mut_val *metadata = yyjson_mut_obj(doc);
  for (const std::int32_t score : value.scores) {
    yyjson_mut_arr_add_sint(doc, scores, score);
  }
  for (const auto &[key, item] : value.metadata) {
    yyjson_mut_obj_add(metadata, text(doc, key), text(doc, item));
  }
  add(doc, object, "id", yyjson_mut_uint(doc, value.id));
  add(doc, object, "name", text(doc, value.name));
  add(doc, object, "active", yyjson_mut_bool(doc, value.active));
  add(doc, object, "scores", scores);
  add(doc, object, "metadata", metadata);
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLeaf &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  add(doc, object, "value", yyjson_mut_sint(doc, value.value));
  add(doc, object, "label", text(doc, value.label));
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLevel4 &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  add(doc, object, "child", make_value(doc, value.child));
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLevel3 &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  add(doc, object, "child", make_value(doc, value.child));
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedLevel2 &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  add(doc, object, "child", make_value(doc, value.child));
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const NestedRecord &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  add(doc, object, "child", make_value(doc, value.child));
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const StringRecord &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  add(doc, object, "payload", text(doc, value.payload));
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const ArrayRecord &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  yyjson_mut_val *values = yyjson_mut_arr(doc);
  for (const std::int64_t item : value.values) {
    yyjson_mut_arr_add_sint(doc, values, item);
  }
  add(doc, object, "values", values);
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const MapRecord &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  yyjson_mut_val *values = yyjson_mut_obj(doc);
  for (const auto &[key, item] : value.values) {
    yyjson_mut_obj_add(values, text(doc, key), yyjson_mut_sint(doc, item));
  }
  add(doc, object, "values", values);
  return object;
}

yyjson_mut_val *make_value(yyjson_mut_doc *doc, const EventRecord &value) {
  yyjson_mut_val *object = yyjson_mut_obj(doc);
  add(doc, object, "sequence", yyjson_mut_uint(doc, value.sequence));
  std::visit(
      [&](const auto &event) {
        using T = std::decay_t<decltype(event)>;
        if constexpr (std::is_same_v<T, Quit>) {
          add(doc, object, "event", yyjson_mut_str(doc, "Quit"));
        } else {
          yyjson_mut_val *tag = yyjson_mut_obj(doc);
          yyjson_mut_val *payload = yyjson_mut_obj(doc);
          if constexpr (std::is_same_v<T, KeyDown>) {
            add(doc, payload, "key", yyjson_mut_sint(doc, event.key));
            add(doc, tag, "KeyDown", payload);
          } else {
            add(doc, payload, "x", yyjson_mut_sint(doc, event.x));
            add(doc, payload, "y", yyjson_mut_sint(doc, event.y));
            add(doc, tag, "MouseMove", payload);
          }
          add(doc, object, "event", tag);
        }
      },
      value.event);
  return object;
}

template <typename T> bool encode(const T &value, std::string &output) {
  yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
  if (doc == nullptr) {
    return false;
  }
  yyjson_mut_doc_set_root(doc, make_value(doc, value));
  std::size_t size = 0;
  char *encoded = yyjson_mut_write(doc, 0, &size);
  if (encoded != nullptr) {
    output.assign(encoded, size);
    std::free(encoded);
  }
  yyjson_mut_doc_free(doc);
  return encoded != nullptr;
}

yyjson_val *field(yyjson_val *object, const char *name) {
  return yyjson_is_obj(object) ? yyjson_obj_get(object, name) : nullptr;
}

bool read_u64(yyjson_val *value, std::uint64_t &output) {
  if (yyjson_is_uint(value)) {
    output = yyjson_get_uint(value);
    return true;
  }
  if (yyjson_is_sint(value) && yyjson_get_sint(value) >= 0) {
    output = static_cast<std::uint64_t>(yyjson_get_sint(value));
    return true;
  }
  return false;
}

template <typename T> bool read_signed(yyjson_val *value, T &output) {
  if (yyjson_is_uint(value)) {
    const std::uint64_t number = yyjson_get_uint(value);
    if (number > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) {
      return false;
    }
    output = static_cast<T>(number);
    return true;
  }
  if (!yyjson_is_sint(value)) {
    return false;
  }
  const std::int64_t number = yyjson_get_sint(value);
  if (number < std::numeric_limits<T>::min() ||
      number > std::numeric_limits<T>::max()) {
    return false;
  }
  output = static_cast<T>(number);
  return true;
}

bool read_string(yyjson_val *value, std::string &output) {
  if (!yyjson_is_str(value)) {
    return false;
  }
  output.assign(yyjson_get_str(value), yyjson_get_len(value));
  return true;
}

bool read_value(yyjson_val *value, NestedLeaf &output);
bool read_value(yyjson_val *value, NestedLevel4 &output);
bool read_value(yyjson_val *value, NestedLevel3 &output);
bool read_value(yyjson_val *value, NestedLevel2 &output);

bool read_value(yyjson_val *value, SmallRecord &output) {
  yyjson_val *scores = field(value, "scores");
  yyjson_val *metadata = field(value, "metadata");
  if (!read_u64(field(value, "id"), output.id) ||
      !read_string(field(value, "name"), output.name) ||
      !yyjson_is_bool(field(value, "active")) || !yyjson_is_arr(scores) ||
      !yyjson_is_obj(metadata)) {
    return false;
  }
  output.active = yyjson_get_bool(field(value, "active"));
  output.scores.clear();
  output.scores.reserve(yyjson_arr_size(scores));
  std::size_t index = 0;
  std::size_t count = 0;
  yyjson_val *item = nullptr;
  yyjson_arr_foreach(scores, index, count, item) {
    std::int32_t score = 0;
    if (!read_signed(item, score)) {
      return false;
    }
    output.scores.push_back(score);
  }
  output.metadata.clear();
  yyjson_val *key = nullptr;
  yyjson_obj_foreach(metadata, index, count, key, item) {
    std::string key_text;
    std::string item_text;
    if (!read_string(key, key_text) || !read_string(item, item_text)) {
      return false;
    }
    output.metadata.emplace(std::move(key_text), std::move(item_text));
  }
  return true;
}

bool read_value(yyjson_val *value, NestedLeaf &output) {
  return read_signed(field(value, "value"), output.value) &&
         read_string(field(value, "label"), output.label);
}

bool read_value(yyjson_val *value, NestedLevel4 &output) {
  return read_value(field(value, "child"), output.child);
}

bool read_value(yyjson_val *value, NestedLevel3 &output) {
  return read_value(field(value, "child"), output.child);
}

bool read_value(yyjson_val *value, NestedLevel2 &output) {
  return read_value(field(value, "child"), output.child);
}

bool read_value(yyjson_val *value, NestedRecord &output) {
  return read_value(field(value, "child"), output.child);
}

bool read_value(yyjson_val *value, StringRecord &output) {
  return read_string(field(value, "payload"), output.payload);
}

bool read_value(yyjson_val *value, ArrayRecord &output) {
  yyjson_val *values = field(value, "values");
  if (!yyjson_is_arr(values)) {
    return false;
  }
  output.values.clear();
  output.values.reserve(yyjson_arr_size(values));
  std::size_t index = 0;
  std::size_t count = 0;
  yyjson_val *item = nullptr;
  yyjson_arr_foreach(values, index, count, item) {
    std::int64_t number = 0;
    if (!read_signed(item, number)) {
      return false;
    }
    output.values.push_back(number);
  }
  return true;
}

bool read_value(yyjson_val *value, MapRecord &output) {
  yyjson_val *values = field(value, "values");
  if (!yyjson_is_obj(values)) {
    return false;
  }
  output.values.clear();
  std::size_t index = 0;
  std::size_t count = 0;
  yyjson_val *key = nullptr;
  yyjson_val *item = nullptr;
  yyjson_obj_foreach(values, index, count, key, item) {
    std::string key_text;
    std::int64_t number = 0;
    if (!read_string(key, key_text) || !read_signed(item, number)) {
      return false;
    }
    output.values.emplace(std::move(key_text), number);
  }
  return true;
}

bool read_value(yyjson_val *value, EventRecord &output) {
  yyjson_val *event = field(value, "event");
  if (!read_u64(field(value, "sequence"), output.sequence)) {
    return false;
  }
  if (yyjson_is_str(event) &&
      std::string_view(yyjson_get_str(event), yyjson_get_len(event)) ==
          "Quit") {
    output.event = Quit{};
    return true;
  }
  yyjson_val *key_down = field(event, "KeyDown");
  if (key_down != nullptr) {
    KeyDown decoded;
    if (!read_signed(field(key_down, "key"), decoded.key)) {
      return false;
    }
    output.event = decoded;
    return true;
  }
  yyjson_val *mouse_move = field(event, "MouseMove");
  MouseMove decoded;
  if (mouse_move == nullptr ||
      !read_signed(field(mouse_move, "x"), decoded.x) ||
      !read_signed(field(mouse_move, "y"), decoded.y)) {
    return false;
  }
  output.event = decoded;
  return true;
}

template <typename T> bool decode(const std::string &source, T &output) {
  yyjson_doc *doc = yyjson_read(source.data(), source.size(), 0);
  if (doc == nullptr) {
    return false;
  }
  const bool ok = read_value(yyjson_doc_get_root(doc), output);
  yyjson_doc_free(doc);
  return ok;
}

} // namespace

#define SERDDE_YYJSON_CODEC(Type)                                              \
  bool yyjson_encode(const Type &value, std::string &output) {                 \
    return encode(value, output);                                              \
  }                                                                            \
  bool yyjson_decode(const std::string &source, Type &output) {                \
    return decode(source, output);                                             \
  }

SERDDE_YYJSON_CODEC(SmallRecord)
SERDDE_YYJSON_CODEC(NestedRecord)
SERDDE_YYJSON_CODEC(StringRecord)
SERDDE_YYJSON_CODEC(ArrayRecord)
SERDDE_YYJSON_CODEC(MapRecord)
SERDDE_YYJSON_CODEC(EventRecord)

#undef SERDDE_YYJSON_CODEC

} // namespace serdde_benchmark
