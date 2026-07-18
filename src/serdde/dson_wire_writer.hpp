#pragma once

#include "json_native.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace serdde_wire {

class DsonWriter {
public:
  std::string format_name() const { return "dson"; }
  std::string error_message() const { return error_; }
  std::string output() const { return output_; }

  bool write_null() { return scalar("null"); }
  bool write_bool(bool value) {
    return scalar(value ? "bool(true)" : "bool(false)");
  }
  bool write_i64(std::int64_t value) { return integer("i64(", value); }
  bool write_u64(std::uint64_t value) { return integer("u64(", value); }

  bool write_f64(double value) {
    if (!serdde_json_native::finite_f64(value)) {
      return fail("DSON cannot encode a non-finite float");
    }
    const std::string spelling = serdde_json_native::format_f64(value);
    if (spelling.empty()) {
      return fail("failed to format floating-point value");
    }
    if (!before_value()) {
      return false;
    }
    output_ += "f64(";
    output_ += spelling;
    output_.push_back(')');
    return true;
  }

  bool write_string(const std::string &value) {
    if (!serdde_json_native::valid_utf8(value)) {
      return fail("string is not valid UTF-8");
    }
    if (!before_value()) {
      return false;
    }
    append_string(value);
    return true;
  }

  bool begin_sequence(std::size_t size) {
    if (!before_value()) {
      return false;
    }
    output_ += "seq(";
    append_size(size);
    output_ += ":[";
    frames_.push_back(Frame{FrameKind::Sequence});
    return true;
  }

  bool begin_object(std::size_t size) { return begin_object(size, false); }
  bool begin_object_checked(std::size_t size) {
    return begin_object(size, true);
  }

  bool write_field(const std::string &name) {
    if (frames_.empty() || frames_.back().kind != FrameKind::Object) {
      return fail("field name was written outside an object");
    }
    Frame &frame = frames_.back();
    if (frame.expecting_value) {
      return fail("object field is missing its value");
    }
    if (frame.reject_duplicate_fields && duplicate_field(frame, name)) {
      return fail("duplicate object field: " + name);
    }
    if (!frame.first) {
      output_.push_back(',');
    }
    frame.first = false;
    append_string(name);
    output_.push_back('=');
    frame.expecting_value = true;
    return true;
  }

  bool end_sequence() { return end(FrameKind::Sequence, "])"); }
  bool end_object() { return end(FrameKind::Object, "})"); }

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

  template <typename Integer>
  bool integer(std::string_view prefix, Integer value) {
    if (!before_value()) {
      return false;
    }
    output_ += prefix;
    char buffer[32];
    const auto [end, error] =
        std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (error != std::errc{}) {
      return fail("failed to format integer value");
    }
    output_.append(buffer, end);
    output_.push_back(')');
    return true;
  }

  void append_size(std::size_t value) {
    char buffer[32];
    const auto [end, error] =
        std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (error == std::errc{}) {
      output_.append(buffer, end);
    }
  }

  void append_string(std::string_view value) {
    output_ += "str(";
    append_size(value.size());
    output_.push_back(':');
    output_ += value;
    output_.push_back(')');
  }

  bool begin_object(std::size_t size, bool checked) {
    if (!before_value()) {
      return false;
    }
    output_ += "obj(";
    append_size(size);
    output_ += ":{";
    frames_.push_back(Frame{FrameKind::Object});
    Frame &frame = frames_.back();
    frame.reject_duplicate_fields = checked;
    if (checked) {
      frame.field_ranges.reserve(size);
      if (size <= std::numeric_limits<std::size_t>::max() / 16) {
        frame.field_names.reserve(size * 16);
      }
    }
    return true;
  }

  bool duplicate_field(Frame &frame, std::string_view name) {
    for (const auto [offset, length] : frame.field_ranges) {
      if (length == name.size() &&
          frame.field_names.compare(offset, length, name) == 0) {
        return true;
      }
    }
    frame.field_ranges.emplace_back(frame.field_names.size(), name.size());
    frame.field_names += name;
    return false;
  }

  bool end(FrameKind kind, std::string_view spelling) {
    if (frames_.empty() || frames_.back().kind != kind) {
      return fail("container close does not match its open");
    }
    if (frames_.back().expecting_value) {
      return fail("object field is missing its value");
    }
    frames_.pop_back();
    output_ += spelling;
    return true;
  }
};

} // namespace serdde_wire
