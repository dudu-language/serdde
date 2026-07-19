#pragma once

#include "json_wire_location.hpp"
#include "yyjson.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace serdde_wire::detail {

using JsonObjectEntry = std::pair<yyjson_val *, yyjson_val *>;
using JsonObjectIndex =
    std::unordered_map<yyjson_val *, std::vector<JsonObjectEntry>>;

inline bool validate_json_tree(yyjson_val *value, yyjson_val *root,
                               std::string_view source, std::size_t depth,
                               std::string &error, std::size_t &error_offset,
                               JsonObjectIndex &object_index) {
  if (depth > 128) {
    error = "JSON nesting depth exceeds 128";
    return false;
  }
  if (yyjson_is_arr(value)) {
    const std::size_t size = yyjson_arr_size(value);
    for (std::size_t index = 0; index < size; ++index) {
      if (!validate_json_tree(yyjson_arr_get(value, index), root, source,
                              depth + 1, error, error_offset, object_index)) {
        return false;
      }
    }
    return true;
  }
  if (!yyjson_is_obj(value)) {
    return true;
  }

  std::vector<std::string_view> keys;
  std::vector<JsonObjectEntry> entries;
  const std::size_t size = yyjson_obj_size(value);
  keys.reserve(size);
  entries.reserve(size);
  yyjson_obj_iter iterator = yyjson_obj_iter_with(value);
  yyjson_val *key = nullptr;
  while ((key = yyjson_obj_iter_next(&iterator)) != nullptr) {
    yyjson_val *child = yyjson_obj_iter_get_val(key);
    keys.emplace_back(yyjson_get_str(key), yyjson_get_len(key));
    entries.emplace_back(key, child);
    if (!validate_json_tree(child, root, source, depth + 1, error, error_offset,
                            object_index)) {
      return false;
    }
  }
  std::sort(keys.begin(), keys.end());
  const auto duplicate = std::adjacent_find(keys.begin(), keys.end());
  if (duplicate != keys.end()) {
    error = "duplicate object key";
    bool seen = false;
    for (const JsonObjectEntry &entry : entries) {
      const std::string_view key(yyjson_get_str(entry.first),
                                 yyjson_get_len(entry.first));
      if (key != *duplicate) {
        continue;
      }
      if (seen) {
        error_offset = json_value_offset(source, root, entry.first);
        break;
      }
      seen = true;
    }
    return false;
  }
  object_index.emplace(value, std::move(entries));
  return true;
}

struct JsonDocument {
  std::string_view source;
  yyjson_doc *document = nullptr;
  std::string error;
  std::size_t error_offset = 0;
  JsonObjectIndex object_index;

  explicit JsonDocument(const std::string &input) : source(input) {
    yyjson_read_err parse_error{};
    // YYJSON_READ_NOFLAG guarantees yyjson will not modify this buffer.
    document =
        yyjson_read_opts(const_cast<char *>(source.data()), source.size(),
                         YYJSON_READ_NOFLAG, nullptr, &parse_error);
    if (document == nullptr) {
      error = parse_error.msg == nullptr ? "invalid JSON" : parse_error.msg;
      error_offset = parse_error.pos;
      return;
    }
    yyjson_val *root = yyjson_doc_get_root(document);
    if (!validate_json_tree(root, root, source, 0, error, error_offset,
                            object_index)) {
      yyjson_doc_free(document);
      document = nullptr;
    }
  }

  ~JsonDocument() {
    if (document != nullptr) {
      yyjson_doc_free(document);
    }
  }

  const JsonObjectEntry *object_entry(yyjson_val *object,
                                      std::size_t index) const {
    const auto entries = object_index.find(object);
    if (entries == object_index.end() || index >= entries->second.size()) {
      return nullptr;
    }
    return &entries->second[index];
  }

  std::size_t line() const {
    std::size_t result = 1;
    const std::size_t limit = std::min(error_offset, source.size());
    for (std::size_t index = 0; index < limit; ++index) {
      if (source[index] == '\n') {
        ++result;
      }
    }
    return result;
  }

  std::size_t column() const {
    const std::size_t limit = std::min(error_offset, source.size());
    const std::size_t newline = source.rfind('\n', limit == 0 ? 0 : limit - 1);
    return newline == std::string::npos ? limit + 1 : limit - newline;
  }

  std::size_t value_offset(yyjson_val *value) const {
    return document == nullptr
               ? error_offset
               : json_value_offset(source, yyjson_doc_get_root(document),
                                   value);
  }
};

} // namespace serdde_wire::detail
