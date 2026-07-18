#pragma once

#include "json_wire_location.hpp"
#include "yyjson.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace serdde_wire {

namespace detail {

inline bool append_escaped_json(std::string &output, std::string_view value) {
  output.push_back('"');
  constexpr char digits[] = "0123456789abcdef";
  for (const unsigned char ch : value) {
    switch (ch) {
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
      if (ch < 0x20) {
        output += "\\u00";
        output.push_back(digits[ch >> 4]);
        output.push_back(digits[ch & 0x0f]);
      } else {
        output.push_back(static_cast<char>(ch));
      }
    }
  }
  output.push_back('"');
  return true;
}

inline bool validate_json_tree(yyjson_val *value, std::size_t depth,
                               std::string &error) {
  if (depth > 128) {
    error = "JSON nesting depth exceeds 128";
    return false;
  }
  if (yyjson_is_arr(value)) {
    const std::size_t size = yyjson_arr_size(value);
    for (std::size_t i = 0; i < size; ++i) {
      if (!validate_json_tree(yyjson_arr_get(value, i), depth + 1, error)) {
        return false;
      }
    }
    return true;
  }
  if (!yyjson_is_obj(value)) {
    return true;
  }
  std::vector<std::string_view> keys;
  keys.reserve(yyjson_obj_size(value));
  yyjson_obj_iter iterator = yyjson_obj_iter_with(value);
  yyjson_val *key = nullptr;
  while ((key = yyjson_obj_iter_next(&iterator)) != nullptr) {
    const std::string_view spelling(yyjson_get_str(key), yyjson_get_len(key));
    for (const std::string_view previous : keys) {
      if (previous == spelling) {
        error = "duplicate object key";
        return false;
      }
    }
    keys.push_back(spelling);
    if (!validate_json_tree(yyjson_obj_iter_get_val(key), depth + 1, error)) {
      return false;
    }
  }
  return true;
}

struct JsonDocument {
  std::string source;
  yyjson_doc *document = nullptr;
  std::string error;
  std::size_t error_offset = 0;

  explicit JsonDocument(std::string input) : source(std::move(input)) {
    yyjson_read_err parse_error{};
    document = yyjson_read_opts(source.data(), source.size(),
                                YYJSON_READ_NOFLAG, nullptr, &parse_error);
    if (document == nullptr) {
      error = parse_error.msg == nullptr ? "invalid JSON" : parse_error.msg;
      error_offset = parse_error.pos;
      return;
    }
    if (!validate_json_tree(yyjson_doc_get_root(document), 0, error)) {
      yyjson_doc_free(document);
      document = nullptr;
    }
  }

  ~JsonDocument() {
    if (document != nullptr) {
      yyjson_doc_free(document);
    }
  }

  std::size_t line() const {
    std::size_t result = 1;
    const std::size_t limit = std::min(error_offset, source.size());
    for (std::size_t i = 0; i < limit; ++i) {
      if (source[i] == '\n') {
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

} // namespace detail

class JsonWriter {
public:
  std::string format_name() const { return "json"; }
  std::string error_message() const { return error_; }
  std::string output() const { return output_; }

  bool write_null() { return scalar("null"); }
  bool write_bool(bool value) { return scalar(value ? "true" : "false"); }

  bool write_i64(std::int64_t value) {
    char buffer[32];
    const auto [end, error] =
        std::to_chars(buffer, buffer + sizeof(buffer), value);
    return error == std::errc{} && scalar(std::string_view(buffer, end));
  }

  bool write_u64(std::uint64_t value) {
    char buffer[32];
    const auto [end, error] =
        std::to_chars(buffer, buffer + sizeof(buffer), value);
    return error == std::errc{} && scalar(std::string_view(buffer, end));
  }

  bool write_f64(double value) {
    if (!std::isfinite(value)) {
      return fail("JSON cannot encode a non-finite float");
    }
    char buffer[128];
    const auto [end, error] = std::to_chars(
        buffer, buffer + sizeof(buffer), value, std::chars_format::general,
        std::numeric_limits<double>::max_digits10);
    if (error != std::errc{}) {
      return fail("failed to format floating-point value");
    }
    if (!before_value()) {
      return false;
    }
    const std::string_view spelling(buffer,
                                    static_cast<std::size_t>(end - buffer));
    output_ += spelling;
    if (spelling.find_first_of(".eE") == std::string_view::npos) {
      output_ += ".0";
    }
    return true;
  }

  bool write_string(const std::string &value) {
    if (!before_value()) {
      return false;
    }
    return detail::append_escaped_json(output_, value);
  }

  bool begin_sequence(std::size_t) { return begin('[', FrameKind::Sequence); }
  bool begin_object(std::size_t) { return begin('{', FrameKind::Object); }
  bool begin_object_checked(std::size_t size) {
    if (!begin('{', FrameKind::Object)) {
      return false;
    }
    Frame &frame = frames_.back();
    frame.reject_duplicate_fields = true;
    frame.field_ranges.reserve(size);
    if (size <= std::numeric_limits<std::size_t>::max() / 16) {
      frame.field_names.reserve(size * 16);
    }
    return true;
  }

  bool write_field(const std::string &name) {
    if (frames_.empty() || frames_.back().kind != FrameKind::Object) {
      return fail("field name was written outside an object");
    }
    Frame &frame = frames_.back();
    if (frame.expecting_value) {
      return fail("object field is missing its value");
    }
    if (frame.reject_duplicate_fields) {
      for (const auto [offset, length] : frame.field_ranges) {
        if (length == name.size() &&
            frame.field_names.compare(offset, length, name) == 0) {
          return fail("duplicate object field: " + name);
        }
      }
      frame.field_ranges.emplace_back(frame.field_names.size(), name.size());
      frame.field_names += name;
    }
    if (!frame.first) {
      output_.push_back(',');
    }
    frame.first = false;
    detail::append_escaped_json(output_, name);
    output_.push_back(':');
    frame.expecting_value = true;
    return true;
  }

  bool end_sequence() { return end(']', FrameKind::Sequence); }
  bool end_object() { return end('}', FrameKind::Object); }

private:
  enum class FrameKind { Sequence, Object };
  struct Frame {
    FrameKind kind;
    bool first = true;
    bool expecting_value = false;
    bool reject_duplicate_fields = false;
    std::string field_names;
    std::vector<std::pair<std::size_t, std::size_t>> field_ranges;
  };

  std::string output_;
  std::vector<Frame> frames_;
  std::string error_;
  bool wrote_root_ = false;

  bool fail(std::string message) {
    error_ = std::move(message);
    return false;
  }

  bool before_value() {
    if (frames_.empty()) {
      if (wrote_root_) {
        return fail("multiple root values were written");
      }
      wrote_root_ = true;
      return true;
    }
    Frame &frame = frames_.back();
    if (frame.kind == FrameKind::Object) {
      if (!frame.expecting_value) {
        return fail("object value was written without a field name");
      }
      frame.expecting_value = false;
      return true;
    }
    if (!frame.first) {
      output_.push_back(',');
    }
    frame.first = false;
    return true;
  }

  bool scalar(std::string_view spelling) {
    if (!before_value()) {
      return false;
    }
    output_ += spelling;
    return true;
  }

  bool begin(char spelling, FrameKind kind) {
    if (!before_value()) {
      return false;
    }
    output_.push_back(spelling);
    frames_.push_back(Frame{kind});
    return true;
  }

  bool end(char spelling, FrameKind kind) {
    if (frames_.empty() || frames_.back().kind != kind) {
      return fail("container close does not match its open");
    }
    if (frames_.back().expecting_value) {
      return fail("object field is missing its value");
    }
    frames_.pop_back();
    output_.push_back(spelling);
    return true;
  }
};

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
    if (!yyjson_is_obj(value_))
      return {};
    yyjson_obj_iter iterator = yyjson_obj_iter_with(value_);
    yyjson_val *key = nullptr;
    std::size_t current = 0;
    while ((key = yyjson_obj_iter_next(&iterator)) != nullptr) {
      if (current++ == index) {
        return std::string(yyjson_get_str(key), yyjson_get_len(key));
      }
    }
    return {};
  }

  JsonReader element(std::size_t index) const {
    if (yyjson_is_arr(value_)) {
      return JsonReader(document_, yyjson_arr_get(value_, index));
    }
    if (yyjson_is_obj(value_)) {
      yyjson_obj_iter iterator = yyjson_obj_iter_with(value_);
      yyjson_val *key = nullptr;
      std::size_t current = 0;
      while ((key = yyjson_obj_iter_next(&iterator)) != nullptr) {
        if (current++ == index) {
          return JsonReader(document_, yyjson_obj_iter_get_val(key));
        }
      }
    }
    return JsonReader(document_, nullptr);
  }

  bool has_field(const std::string &name) const {
    return yyjson_is_obj(value_) &&
           yyjson_obj_getn(value_, name.data(), name.size()) != nullptr;
  }

  JsonReader field(const std::string &name) const {
    return JsonReader(document_,
                      yyjson_is_obj(value_)
                          ? yyjson_obj_getn(value_, name.data(), name.size())
                          : nullptr);
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
