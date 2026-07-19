#pragma once

#include "json_native.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace serdde_wire {
namespace detail {

inline void append_escaped_json(std::string &output, std::string_view value) {
  output.push_back('"');
  constexpr char digits[] = "0123456789abcdef";
  std::size_t segment_begin = 0;
  for (std::size_t index = 0; index < value.size(); ++index) {
    const unsigned char ch = static_cast<unsigned char>(value[index]);
    if (ch >= 0x20 && ch != '"' && ch != '\\') {
      continue;
    }
    output.append(value.data() + segment_begin, index - segment_begin);
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
      }
    }
    segment_begin = index + 1;
  }
  output.append(value.data() + segment_begin, value.size() - segment_begin);
  output.push_back('"');
}

} // namespace detail

class JsonWriter {
public:
  JsonWriter() : output_(&owned_output_) {}
  explicit JsonWriter(std::string &output) : output_(&output) {
    output.clear();
  }

  std::string format_name() const { return "json"; }
  std::string error_message() const { return error_; }
  std::string output() const { return *output_; }
  std::string take_output() { return std::move(*output_); }
  void reserve(std::size_t size) { output_->reserve(size); }
  bool canonical_object_order() const { return false; }

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
    *output_ += spelling;
    if (spelling.find_first_of(".eE") == std::string_view::npos) {
      *output_ += ".0";
    }
    return true;
  }

  bool write_string(const std::string &value) {
    if (!serdde_json_native::valid_utf8(value)) {
      return fail("string is not valid UTF-8");
    }
    if (!before_value()) {
      return false;
    }
    detail::append_escaped_json(*output_, value);
    return true;
  }

  bool begin_sequence(std::size_t) { return begin('[', FrameKind::Sequence); }
  bool begin_set(std::size_t size) { return begin_sequence(size); }
  bool begin_object(std::size_t) { return begin('{', FrameKind::Object); }
  bool begin_object_ordered(std::size_t size) { return begin_object(size); }
  bool begin_object_compact(std::size_t size) { return begin_object(size); }
  bool begin_object_compact_ordered(std::size_t size) {
    return begin_object(size);
  }

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
    if (!serdde_json_native::valid_utf8(name)) {
      return fail("object field is not valid UTF-8");
    }
    if (frame.reject_duplicate_fields && !record_field(frame, name)) {
      return false;
    }
    if (!frame.first) {
      output_->push_back(',');
    }
    frame.first = false;
    detail::append_escaped_json(*output_, name);
    output_->push_back(':');
    frame.expecting_value = true;
    return true;
  }

  bool write_field_id(const std::string &name, std::uint64_t) {
    return write_field(name);
  }

  bool end_sequence() { return end(']', FrameKind::Sequence); }
  bool end_object() { return end('}', FrameKind::Object); }

private:
  enum class FrameKind { Sequence, Object };

  struct Frame {
    explicit Frame(FrameKind frame_kind) : kind(frame_kind) {}

    FrameKind kind;
    bool first = true;
    bool expecting_value = false;
    bool reject_duplicate_fields = false;
    std::string field_names;
    std::vector<std::pair<std::size_t, std::size_t>> field_ranges;
  };

  std::string owned_output_;
  std::string *output_;
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
      output_->push_back(',');
    }
    frame.first = false;
    return true;
  }

  bool scalar(std::string_view spelling) {
    if (!before_value()) {
      return false;
    }
    *output_ += spelling;
    return true;
  }

  bool begin(char spelling, FrameKind kind) {
    if (!before_value()) {
      return false;
    }
    output_->push_back(spelling);
    frames_.emplace_back(kind);
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
    output_->push_back(spelling);
    return true;
  }

  bool record_field(Frame &frame, const std::string &name) {
    for (const auto &[offset, length] : frame.field_ranges) {
      if (length == name.size() &&
          frame.field_names.compare(offset, length, name) == 0) {
        return fail("duplicate object field: " + name);
      }
    }
    frame.field_ranges.emplace_back(frame.field_names.size(), name.size());
    frame.field_names += name;
    return true;
  }
};

} // namespace serdde_wire
