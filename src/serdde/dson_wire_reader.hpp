#pragma once

#include "json_native.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace serdde_wire {
namespace dson_detail {

enum class Kind {
  Invalid,
  Null,
  Bool,
  I64,
  U64,
  F64,
  String,
  Sequence,
  Object
};

struct Node {
  Kind kind = Kind::Invalid;
  std::size_t begin = 0;
  std::size_t end = 0;
  std::size_t payload_begin = 0;
  std::size_t payload_end = 0;
  std::size_t count = 0;
};

struct Cursor {
  std::string_view source;
  std::size_t offset = 0;
  std::string error;
  std::size_t error_offset = 0;

  bool fail(std::string message) {
    if (error.empty()) {
      error = std::move(message);
      error_offset = offset;
    }
    return false;
  }

  void skip_space() {
    while (offset < source.size()) {
      const char value = source[offset];
      if (value != ' ' && value != '\t' && value != '\n' && value != '\r') {
        return;
      }
      ++offset;
    }
  }

  bool consume(char expected, std::string_view message) {
    if (offset >= source.size() || source[offset] != expected) {
      return fail(std::string(message));
    }
    ++offset;
    return true;
  }

  bool consume(std::string_view expected, std::string_view message) {
    if (source.substr(offset, expected.size()) != expected) {
      return fail(std::string(message));
    }
    offset += expected.size();
    return true;
  }
};

inline bool parse_value(Cursor &cursor, std::size_t depth, Node &output);

inline bool parse_size(Cursor &cursor, std::size_t &output) {
  const std::size_t begin = cursor.offset;
  while (cursor.offset < cursor.source.size() &&
         cursor.source[cursor.offset] >= '0' &&
         cursor.source[cursor.offset] <= '9') {
    ++cursor.offset;
  }
  if (cursor.offset == begin) {
    return cursor.fail("expected a non-negative integer");
  }
  const char *first = cursor.source.data() + begin;
  const char *last = cursor.source.data() + cursor.offset;
  const auto [end, error] = std::from_chars(first, last, output, 10);
  return error == std::errc{} && end == last
             ? true
             : cursor.fail("integer is out of range");
}

template <typename Number>
inline bool parse_integer(std::string_view text, Number &output) {
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), output, 10);
  return error == std::errc{} && end == text.data() + text.size();
}

inline bool parse_float(std::string_view text, double &output) {
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), output,
                      std::chars_format::general);
  return error == std::errc{} && end == text.data() + text.size() &&
         std::isfinite(output);
}

inline bool parse_numeric(Cursor &cursor, std::string_view prefix, Kind kind,
                          Node &output) {
  if (!cursor.consume(prefix, "invalid DSON numeric tag")) {
    return false;
  }
  output.payload_begin = cursor.offset;
  const std::size_t close = cursor.source.find(')', cursor.offset);
  if (close == std::string_view::npos) {
    return cursor.fail("unterminated numeric value");
  }
  output.payload_end = close;
  const std::string_view spelling =
      cursor.source.substr(output.payload_begin, close - output.payload_begin);
  bool valid = false;
  if (kind == Kind::I64) {
    std::int64_t value = 0;
    valid = parse_integer(spelling, value);
  } else if (kind == Kind::U64) {
    std::uint64_t value = 0;
    valid = parse_integer(spelling, value);
  } else {
    double value = 0.0;
    valid = parse_float(spelling, value);
  }
  if (!valid) {
    return cursor.fail("invalid DSON numeric value");
  }
  cursor.offset = close + 1;
  output.kind = kind;
  output.end = cursor.offset;
  return true;
}

inline bool parse_string(Cursor &cursor, Node &output) {
  if (!cursor.consume("str(", "invalid DSON string tag")) {
    return false;
  }
  std::size_t length = 0;
  if (!parse_size(cursor, length) ||
      !cursor.consume(':', "expected string length separator")) {
    return false;
  }
  if (length > cursor.source.size() - cursor.offset) {
    return cursor.fail("string length exceeds remaining input");
  }
  output.payload_begin = cursor.offset;
  output.payload_end = cursor.offset + length;
  const std::string_view text = cursor.source.substr(cursor.offset, length);
  if (!serdde_json_native::valid_utf8(text)) {
    return cursor.fail("string is not valid UTF-8");
  }
  cursor.offset += length;
  if (!cursor.consume(')', "unterminated string value")) {
    return false;
  }
  output.kind = Kind::String;
  output.end = cursor.offset;
  return true;
}

inline bool duplicate_key(Cursor &cursor, std::size_t entries_begin,
                          std::size_t prior_count, const Node &candidate,
                          std::size_t depth) {
  Cursor previous{cursor.source, entries_begin};
  for (std::size_t i = 0; i < prior_count; ++i) {
    previous.skip_space();
    Node key;
    if (!parse_value(previous, depth + 1, key) || key.kind != Kind::String) {
      return cursor.fail("invalid prior object key");
    }
    const std::string_view left = previous.source.substr(
        key.payload_begin, key.payload_end - key.payload_begin);
    const std::string_view right =
        cursor.source.substr(candidate.payload_begin,
                             candidate.payload_end - candidate.payload_begin);
    if (left == right) {
      cursor.error_offset = candidate.begin;
      return cursor.fail("duplicate object key");
    }
    previous.skip_space();
    if (!previous.consume('=', "expected object field separator")) {
      return cursor.fail(previous.error);
    }
    previous.skip_space();
    Node value;
    if (!parse_value(previous, depth + 1, value)) {
      return cursor.fail(previous.error);
    }
    previous.skip_space();
    if (i + 1 < prior_count &&
        !previous.consume(',', "expected object entry separator")) {
      return cursor.fail(previous.error);
    }
  }
  return true;
}

inline bool parse_sequence(Cursor &cursor, std::size_t depth, Node &output) {
  if (depth >= 128) {
    return cursor.fail("DSON nesting depth exceeds 128");
  }
  if (!cursor.consume("seq(", "invalid DSON sequence tag") ||
      !parse_size(cursor, output.count) ||
      !cursor.consume(':', "expected sequence count separator") ||
      !cursor.consume('[', "expected sequence body")) {
    return false;
  }
  output.kind = Kind::Sequence;
  output.payload_begin = cursor.offset;
  for (std::size_t i = 0; i < output.count; ++i) {
    cursor.skip_space();
    Node child;
    if (!parse_value(cursor, depth + 1, child)) {
      return false;
    }
    cursor.skip_space();
    if (i + 1 < output.count &&
        !cursor.consume(',', "expected sequence entry separator")) {
      return false;
    }
  }
  output.payload_end = cursor.offset;
  if (!cursor.consume(']', "sequence count does not match its body") ||
      !cursor.consume(')', "unterminated sequence value")) {
    return false;
  }
  output.end = cursor.offset;
  return true;
}

inline bool parse_object(Cursor &cursor, std::size_t depth, Node &output) {
  if (depth >= 128) {
    return cursor.fail("DSON nesting depth exceeds 128");
  }
  if (!cursor.consume("obj(", "invalid DSON object tag") ||
      !parse_size(cursor, output.count) ||
      !cursor.consume(':', "expected object count separator") ||
      !cursor.consume('{', "expected object body")) {
    return false;
  }
  output.kind = Kind::Object;
  output.payload_begin = cursor.offset;
  for (std::size_t i = 0; i < output.count; ++i) {
    cursor.skip_space();
    Node key;
    if (!parse_value(cursor, depth + 1, key) || key.kind != Kind::String) {
      return cursor.fail("object key must be a DSON string");
    }
    if (!duplicate_key(cursor, output.payload_begin, i, key, depth)) {
      return false;
    }
    cursor.skip_space();
    if (!cursor.consume('=', "expected object field separator")) {
      return false;
    }
    cursor.skip_space();
    Node value;
    if (!parse_value(cursor, depth + 1, value)) {
      return false;
    }
    cursor.skip_space();
    if (i + 1 < output.count &&
        !cursor.consume(',', "expected object entry separator")) {
      return false;
    }
  }
  output.payload_end = cursor.offset;
  if (!cursor.consume('}', "object count does not match its body") ||
      !cursor.consume(')', "unterminated object value")) {
    return false;
  }
  output.end = cursor.offset;
  return true;
}

inline bool parse_value(Cursor &cursor, std::size_t depth, Node &output) {
  cursor.skip_space();
  output.begin = cursor.offset;
  if (cursor.source.substr(cursor.offset, 4) == "null") {
    cursor.offset += 4;
    output.kind = Kind::Null;
    output.end = cursor.offset;
    return true;
  }
  if (cursor.source.substr(cursor.offset, 10) == "bool(true)") {
    cursor.offset += 10;
    output.kind = Kind::Bool;
    output.payload_begin = output.begin + 5;
    output.payload_end = output.begin + 9;
    output.end = cursor.offset;
    return true;
  }
  if (cursor.source.substr(cursor.offset, 11) == "bool(false)") {
    cursor.offset += 11;
    output.kind = Kind::Bool;
    output.payload_begin = output.begin + 5;
    output.payload_end = output.begin + 10;
    output.end = cursor.offset;
    return true;
  }
  if (cursor.source.substr(cursor.offset, 4) == "i64(") {
    return parse_numeric(cursor, "i64(", Kind::I64, output);
  }
  if (cursor.source.substr(cursor.offset, 4) == "u64(") {
    return parse_numeric(cursor, "u64(", Kind::U64, output);
  }
  if (cursor.source.substr(cursor.offset, 4) == "f64(") {
    return parse_numeric(cursor, "f64(", Kind::F64, output);
  }
  if (cursor.source.substr(cursor.offset, 4) == "str(") {
    return parse_string(cursor, output);
  }
  if (cursor.source.substr(cursor.offset, 4) == "seq(") {
    return parse_sequence(cursor, depth, output);
  }
  if (cursor.source.substr(cursor.offset, 4) == "obj(") {
    return parse_object(cursor, depth, output);
  }
  return cursor.fail("unknown DSON value tag");
}

} // namespace dson_detail

class DsonReader {
public:
  DsonReader() = default;
  explicit DsonReader(const std::string &source) : source_(&source) {
    dson_detail::Cursor cursor{source};
    if (!dson_detail::parse_value(cursor, 0, node_)) {
      error_ = cursor.error;
      error_offset_ = cursor.error_offset;
      node_ = {};
      return;
    }
    cursor.skip_space();
    if (cursor.offset != source.size()) {
      error_ = "trailing characters after DSON value";
      error_offset_ = cursor.offset;
      node_ = {};
    }
  }

  bool valid() const {
    return source_ != nullptr && node_.kind != dson_detail::Kind::Invalid;
  }
  std::string format_name() const { return "dson"; }
  std::string error_message() const { return error_; }
  std::size_t offset() const { return valid() ? node_.begin : error_offset_; }
  std::size_t line() const { return line_and_column().first; }
  std::size_t column() const { return line_and_column().second; }

  bool is_null() const { return node_.kind == dson_detail::Kind::Null; }
  bool is_sequence() const { return node_.kind == dson_detail::Kind::Sequence; }
  bool is_object() const { return node_.kind == dson_detail::Kind::Object; }

  bool get_bool(bool &output) const {
    if (node_.kind != dson_detail::Kind::Bool)
      return false;
    output = (*source_)[node_.payload_begin] == 't';
    return true;
  }
  bool get_i64(std::int64_t &output) const {
    return node_.kind == dson_detail::Kind::I64 &&
           dson_detail::parse_integer(payload(), output);
  }
  bool get_u64(std::uint64_t &output) const {
    return node_.kind == dson_detail::Kind::U64 &&
           dson_detail::parse_integer(payload(), output);
  }
  bool get_f64(double &output) const {
    return node_.kind == dson_detail::Kind::F64 &&
           dson_detail::parse_float(payload(), output);
  }
  bool get_string(std::string &output) const {
    if (node_.kind != dson_detail::Kind::String)
      return false;
    output.assign(source_->data() + node_.payload_begin,
                  node_.payload_end - node_.payload_begin);
    return true;
  }

  std::size_t size() const {
    return is_sequence() || is_object() ? node_.count : 0;
  }
  std::string key(std::size_t index) const {
    dson_detail::Node key_node;
    dson_detail::Node value_node;
    if (!object_entry(index, key_node, value_node))
      return {};
    return std::string(source_->data() + key_node.payload_begin,
                       key_node.payload_end - key_node.payload_begin);
  }
  DsonReader element(std::size_t index) const {
    if (is_sequence()) {
      dson_detail::Node child;
      return sequence_element(index, child) ? DsonReader(source_, child)
                                            : DsonReader(source_, {});
    }
    dson_detail::Node key_node;
    dson_detail::Node value_node;
    return object_entry(index, key_node, value_node)
               ? DsonReader(source_, value_node)
               : DsonReader(source_, dson_detail::Node{});
  }
  bool has_field(const std::string &name) const {
    dson_detail::Node value;
    return find_field(name, value);
  }
  DsonReader field(const std::string &name) const {
    dson_detail::Node value;
    return find_field(name, value) ? DsonReader(source_, value)
                                   : DsonReader(source_, {});
  }

private:
  const std::string *source_ = nullptr;
  dson_detail::Node node_;
  std::string error_;
  std::size_t error_offset_ = 0;

  DsonReader(const std::string *source, dson_detail::Node node)
      : source_(source), node_(node) {}

  std::string_view payload() const {
    return std::string_view(source_->data() + node_.payload_begin,
                            node_.payload_end - node_.payload_begin);
  }

  std::pair<std::size_t, std::size_t> line_and_column() const {
    if (source_ == nullptr)
      return {1, 1};
    const std::size_t limit = std::min(offset(), source_->size());
    std::size_t line = 1;
    std::size_t column = 1;
    for (std::size_t i = 0; i < limit; ++i) {
      if ((*source_)[i] == '\n') {
        ++line;
        column = 1;
      } else {
        ++column;
      }
    }
    return {line, column};
  }

  bool sequence_element(std::size_t index, dson_detail::Node &output) const {
    if (!is_sequence() || index >= node_.count)
      return false;
    dson_detail::Cursor cursor{*source_, node_.payload_begin};
    for (std::size_t i = 0; i <= index; ++i) {
      cursor.skip_space();
      if (!dson_detail::parse_value(cursor, 1, output))
        return false;
      cursor.skip_space();
      if (i < index && !cursor.consume(',', "expected sequence separator")) {
        return false;
      }
    }
    return true;
  }

  bool object_entry(std::size_t index, dson_detail::Node &key,
                    dson_detail::Node &value) const {
    if (!is_object() || index >= node_.count)
      return false;
    dson_detail::Cursor cursor{*source_, node_.payload_begin};
    for (std::size_t i = 0; i <= index; ++i) {
      cursor.skip_space();
      if (!dson_detail::parse_value(cursor, 1, key) ||
          key.kind != dson_detail::Kind::String)
        return false;
      cursor.skip_space();
      if (!cursor.consume('=', "expected object field separator"))
        return false;
      cursor.skip_space();
      if (!dson_detail::parse_value(cursor, 1, value))
        return false;
      cursor.skip_space();
      if (i < index && !cursor.consume(',', "expected object separator")) {
        return false;
      }
    }
    return true;
  }

  bool find_field(std::string_view name, dson_detail::Node &value) const {
    if (!is_object())
      return false;
    for (std::size_t i = 0; i < node_.count; ++i) {
      dson_detail::Node key_node;
      if (!object_entry(i, key_node, value))
        return false;
      const std::string_view key(source_->data() + key_node.payload_begin,
                                 key_node.payload_end - key_node.payload_begin);
      if (key == name)
        return true;
    }
    return false;
  }
};

} // namespace serdde_wire
