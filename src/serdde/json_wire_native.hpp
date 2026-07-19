#pragma once

#include "json_wire_document.hpp"
#include "json_wire_writer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace serdde_wire {

class JsonReader {
public:
  JsonReader() = default;

  explicit JsonReader(const std::string &source)
      : document_(std::make_shared<detail::JsonDocument>(source)) {
    if (document_->document != nullptr) {
      value_ = yyjson_doc_get_root(document_->document);
    }
  }

  bool valid() const { return value_ != nullptr; }
  std::string format_name() const { return "json"; }
  std::string error_message() const {
    return document_ == nullptr ? "invalid JSON reader" : document_->error;
  }
  std::size_t line() const { return line_and_column().first; }
  std::size_t column() const { return line_and_column().second; }
  std::size_t offset() const {
    return document_ == nullptr ? 0 : document_->value_offset(value_);
  }

  bool is_null() const { return yyjson_is_null(value_); }
  bool is_sequence() const { return yyjson_is_arr(value_); }
  bool is_object() const { return yyjson_is_obj(value_); }

  bool get_bool(bool &output) const {
    if (!yyjson_is_bool(value_))
      return false;
    output = yyjson_get_bool(value_);
    return true;
  }

  bool get_i64(std::int64_t &output) const {
    if (yyjson_is_sint(value_)) {
      output = yyjson_get_sint(value_);
      return true;
    }
    if (yyjson_is_uint(value_) &&
        yyjson_get_uint(value_) <=
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max())) {
      output = static_cast<std::int64_t>(yyjson_get_uint(value_));
      return true;
    }
    return false;
  }

  bool get_u64(std::uint64_t &output) const {
    if (yyjson_is_uint(value_)) {
      output = yyjson_get_uint(value_);
      return true;
    }
    if (yyjson_is_sint(value_) && yyjson_get_sint(value_) >= 0) {
      output = static_cast<std::uint64_t>(yyjson_get_sint(value_));
      return true;
    }
    return false;
  }

  bool get_f64(double &output) const {
    if (!yyjson_is_num(value_))
      return false;
    output = yyjson_get_num(value_);
    return std::isfinite(output);
  }

  bool get_string(std::string &output) const {
    if (!yyjson_is_str(value_))
      return false;
    output.assign(yyjson_get_str(value_), yyjson_get_len(value_));
    return true;
  }

  std::size_t size() const {
    if (yyjson_is_arr(value_))
      return yyjson_arr_size(value_);
    if (yyjson_is_obj(value_))
      return yyjson_obj_size(value_);
    return 0;
  }

  std::string key(std::size_t index) const {
    if (!yyjson_is_obj(value_)) {
      return {};
    }
    const detail::JsonObjectEntry *entry =
        document_->object_entry(value_, index);
    if (entry == nullptr) {
      return {};
    }
    return std::string(yyjson_get_str(entry->first),
                       yyjson_get_len(entry->first));
  }

  JsonReader element(std::size_t index) const {
    if (yyjson_is_arr(value_)) {
      return JsonReader(document_, yyjson_arr_get(value_, index));
    }
    if (yyjson_is_obj(value_)) {
      const detail::JsonObjectEntry *entry =
          document_->object_entry(value_, index);
      return JsonReader(document_, entry == nullptr ? nullptr : entry->second);
    }
    return JsonReader(document_, nullptr);
  }

  bool has_field(const std::string &name) const {
    return yyjson_is_obj(value_) &&
           yyjson_obj_getn(value_, name.data(), name.size()) != nullptr;
  }

  std::size_t field_count(const std::string &name) const {
    return has_field(name) ? 1 : 0;
  }

  JsonReader field(const std::string &name) const {
    return JsonReader(document_,
                      yyjson_is_obj(value_)
                          ? yyjson_obj_getn(value_, name.data(), name.size())
                          : nullptr);
  }

  bool has_field_id(const std::string &name, std::uint64_t) const {
    return has_field(name);
  }
  std::size_t field_count_id(const std::string &name, std::uint64_t) const {
    return field_count(name);
  }
  JsonReader field_id(const std::string &name, std::uint64_t) const {
    return field(name);
  }
  bool key_matches(std::size_t index, const std::string &name,
                   std::uint64_t) const {
    const detail::JsonObjectEntry *entry =
        yyjson_is_obj(value_) ? document_->object_entry(value_, index)
                              : nullptr;
    return entry != nullptr && yyjson_get_len(entry->first) == name.size() &&
           std::string_view(yyjson_get_str(entry->first),
                            yyjson_get_len(entry->first)) == name;
  }

private:
  std::shared_ptr<detail::JsonDocument> document_;
  yyjson_val *value_ = nullptr;

  JsonReader(std::shared_ptr<detail::JsonDocument> document, yyjson_val *value)
      : document_(std::move(document)), value_(value) {}

  std::pair<std::size_t, std::size_t> line_and_column() const {
    if (document_ == nullptr)
      return {1, 1};
    const std::size_t limit = std::min(offset(), document_->source.size());
    std::size_t line = 1;
    std::size_t column = 1;
    for (std::size_t i = 0; i < limit; ++i) {
      if (document_->source[i] == '\n') {
        ++line;
        column = 1;
      } else {
        ++column;
      }
    }
    return {line, column};
  }
};

} // namespace serdde_wire
