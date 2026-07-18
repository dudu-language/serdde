#pragma once

#include "cbor_wire_common.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace serdde_wire {

class CborWriter {
public:
  CborWriter() : output_(&owned_output_) {}
  explicit CborWriter(std::string &output) : output_(&output) {
    output.clear();
  }

  std::string format_name() const { return "cbor"; }
  std::string error_message() const { return error_; }
  std::string output() const { return *output_; }
  std::string take_output() { return std::move(*output_); }
  void reserve(std::size_t size) { output_->reserve(size); }
  bool canonical_object_order() const { return true; }

  bool write_null() { return scalar_byte(0xf6); }
  bool write_bool(bool value) { return scalar_byte(value ? 0xf5 : 0xf4); }

  bool write_i64(std::int64_t value) {
    if (!before_value()) {
      return false;
    }
    if (value >= 0) {
      append_head(active_output(), 0, static_cast<std::uint64_t>(value));
    } else {
      append_head(active_output(), 1, static_cast<std::uint64_t>(-(value + 1)));
    }
    return true;
  }

  bool write_u64(std::uint64_t value) {
    if (!before_value()) {
      return false;
    }
    append_head(active_output(), 0, value);
    return true;
  }

  bool write_f64(double value) {
    if (!std::isfinite(value)) {
      return fail("CBOR cannot encode a non-finite float");
    }
    if (!before_value()) {
      return false;
    }
    std::string &target = active_output();
    const float narrowed = static_cast<float>(value);
    if (static_cast<double>(narrowed) == value) {
      std::uint16_t half = 0;
      if (float_to_half(narrowed, half) &&
          cbor_detail::half_to_double(half) == value) {
        target.push_back(static_cast<char>(0xf9));
        append_big_endian(target, half, 2);
        return true;
      }
      target.push_back(static_cast<char>(0xfa));
      append_big_endian(target, std::bit_cast<std::uint32_t>(narrowed), 4);
      return true;
    }
    target.push_back(static_cast<char>(0xfb));
    append_big_endian(target, std::bit_cast<std::uint64_t>(value), 8);
    return true;
  }

  bool write_string(const std::string &value) {
    if (!serdde_json_native::valid_utf8(value)) {
      return fail("string is not valid UTF-8");
    }
    if (!before_value()) {
      return false;
    }
    append_text(active_output(), value);
    return true;
  }

  bool begin_sequence(std::size_t size) {
    if (!before_value()) {
      return false;
    }
    append_head(active_output(), 4, size);
    frames_.emplace_back(FrameKind::Sequence, size, false);
    return true;
  }

  bool begin_set(std::size_t size) {
    if (!before_value()) {
      return false;
    }
    frames_.emplace_back(FrameKind::BufferedSequence, size, false);
    frames_.back().entries.reserve(size);
    return true;
  }

  bool begin_object(std::size_t size) {
    return begin_object(size, FrameKind::BufferedObject, false);
  }
  bool begin_object_checked(std::size_t size) {
    return begin_object(size, FrameKind::BufferedObject, false);
  }
  bool begin_object_ordered(std::size_t size) {
    return begin_object(size, FrameKind::OrderedObject, false);
  }
  bool begin_object_compact(std::size_t size) {
    return begin_object(size, FrameKind::BufferedObject, true);
  }
  bool begin_object_compact_ordered(std::size_t size) {
    return begin_object(size, FrameKind::OrderedObject, true);
  }

  bool write_field(const std::string &name) {
    return write_key(encoded_text(name));
  }

  bool write_field_id(const std::string &name, std::uint64_t id) {
    if (!frames_.empty() && frames_.back().compact) {
      std::string key;
      append_head(key, 0, id);
      return write_key(std::move(key));
    }
    return write_field(name);
  }

  bool end_sequence() {
    if (frames_.empty() || !is_sequence_frame(frames_.back().kind)) {
      return fail("container close does not match its open");
    }
    if (frames_.back().written != frames_.back().expected) {
      return fail("sequence length does not match declared size");
    }
    if (frames_.back().kind == FrameKind::BufferedSequence) {
      Frame frame = std::move(frames_.back());
      frames_.pop_back();
      std::sort(frame.entries.begin(), frame.entries.end(),
                [](const Entry &left, const Entry &right) {
                  if (left.value.size() != right.value.size()) {
                    return left.value.size() < right.value.size();
                  }
                  return unsigned_less(left.value, right.value);
                });
      std::string encoded;
      std::size_t capacity = 9;
      for (const Entry &entry : frame.entries) {
        capacity += entry.value.size();
      }
      encoded.reserve(capacity);
      append_head(encoded, 4, frame.entries.size());
      for (const Entry &entry : frame.entries) {
        encoded += entry.value;
      }
      active_output() += encoded;
      return true;
    }
    frames_.pop_back();
    return true;
  }

  bool end_object() {
    if (frames_.empty() || is_sequence_frame(frames_.back().kind)) {
      return fail("container close does not match its open");
    }
    if (frames_.back().expecting_value) {
      return fail("object field is missing its value");
    }
    if (frames_.back().written != frames_.back().expected) {
      return fail("object length does not match declared size");
    }
    if (frames_.back().kind == FrameKind::OrderedObject) {
      frames_.pop_back();
      return true;
    }

    Frame frame = std::move(frames_.back());
    frames_.pop_back();
    std::sort(frame.entries.begin(), frame.entries.end(),
              [](const Entry &left, const Entry &right) {
                if (left.key.size() != right.key.size()) {
                  return left.key.size() < right.key.size();
                }
                return unsigned_less(left.key, right.key);
              });
    for (std::size_t index = 1; index < frame.entries.size(); ++index) {
      if (frame.entries[index - 1].key == frame.entries[index].key) {
        return fail("duplicate CBOR object key");
      }
    }
    std::string encoded;
    std::size_t capacity = 9;
    for (const Entry &entry : frame.entries) {
      capacity += entry.key.size() + entry.value.size();
    }
    encoded.reserve(capacity);
    append_head(encoded, 5, frame.entries.size());
    for (const Entry &entry : frame.entries) {
      encoded += entry.key;
      encoded += entry.value;
    }
    active_output() += encoded;
    return true;
  }

private:
  enum class FrameKind {
    Sequence,
    BufferedSequence,
    BufferedObject,
    OrderedObject
  };
  struct Entry {
    std::string key;
    std::string value;
  };
  struct Frame {
    Frame(FrameKind frame_kind, std::size_t count, bool compact_keys)
        : kind(frame_kind), expected(count), compact(compact_keys) {}

    FrameKind kind;
    std::size_t expected = 0;
    std::size_t written = 0;
    bool compact = false;
    bool expecting_value = false;
    std::vector<Entry> entries;
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

  static bool is_sequence_frame(FrameKind kind) {
    return kind == FrameKind::Sequence || kind == FrameKind::BufferedSequence;
  }

  static void append_big_endian(std::string &target, std::uint64_t value,
                                std::size_t width) {
    for (std::size_t shift = width; shift > 0; --shift) {
      target.push_back(static_cast<char>(value >> ((shift - 1) * 8)));
    }
  }

  static void append_head(std::string &target, std::uint8_t major,
                          std::uint64_t argument) {
    const auto prefix = static_cast<std::uint8_t>(major << 5);
    if (argument <= 23) {
      target.push_back(static_cast<char>(prefix | argument));
    } else if (argument <= 0xff) {
      target.push_back(static_cast<char>(prefix | 24));
      append_big_endian(target, argument, 1);
    } else if (argument <= 0xffff) {
      target.push_back(static_cast<char>(prefix | 25));
      append_big_endian(target, argument, 2);
    } else if (argument <= 0xffffffffu) {
      target.push_back(static_cast<char>(prefix | 26));
      append_big_endian(target, argument, 4);
    } else {
      target.push_back(static_cast<char>(prefix | 27));
      append_big_endian(target, argument, 8);
    }
  }

  static void append_text(std::string &target, std::string_view value) {
    append_head(target, 3, value.size());
    target += value;
  }

  static std::string encoded_text(std::string_view value) {
    std::string result;
    result.reserve(value.size() + 9);
    append_text(result, value);
    return result;
  }

  static bool unsigned_less(std::string_view left, std::string_view right) {
    for (std::size_t index = 0; index < left.size(); ++index) {
      const auto lhs = static_cast<unsigned char>(left[index]);
      const auto rhs = static_cast<unsigned char>(right[index]);
      if (lhs != rhs) {
        return lhs < rhs;
      }
    }
    return false;
  }

  static bool float_to_half(float value, std::uint16_t &output) {
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    const std::uint32_t sign = (bits >> 16) & 0x8000u;
    const std::uint32_t fraction = bits & 0x7fffffu;
    int exponent = static_cast<int>((bits >> 23) & 0xffu) - 127 + 15;
    std::uint32_t rounded = 0;
    if (exponent <= 0) {
      if (exponent < -10) {
        output = static_cast<std::uint16_t>(sign);
        return true;
      }
      const std::uint32_t mantissa = fraction | 0x800000u;
      const int shift = 14 - exponent;
      rounded = mantissa >> shift;
      const std::uint32_t remainder = mantissa & ((1u << shift) - 1u);
      const std::uint32_t halfway = 1u << (shift - 1);
      if (remainder > halfway ||
          (remainder == halfway && (rounded & 1u) != 0)) {
        ++rounded;
      }
      output = static_cast<std::uint16_t>(sign | rounded);
      return true;
    }
    if (exponent >= 31) {
      output = static_cast<std::uint16_t>(sign | 0x7c00u);
      return true;
    }
    rounded = fraction >> 13;
    const std::uint32_t remainder = fraction & 0x1fffu;
    if (remainder > 0x1000u || (remainder == 0x1000u && (rounded & 1u) != 0)) {
      ++rounded;
      if (rounded == 0x400u) {
        rounded = 0;
        ++exponent;
        if (exponent >= 31) {
          output = static_cast<std::uint16_t>(sign | 0x7c00u);
          return true;
        }
      }
    }
    output = static_cast<std::uint16_t>(
        sign | (static_cast<std::uint32_t>(exponent) << 10) | rounded);
    return true;
  }

  std::string &active_output() {
    for (auto frame = frames_.rbegin(); frame != frames_.rend(); ++frame) {
      if (frame->kind == FrameKind::BufferedSequence &&
          !frame->entries.empty()) {
        return frame->entries.back().value;
      }
      if (frame->kind == FrameKind::BufferedObject && !frame->expecting_value &&
          !frame->entries.empty()) {
        return frame->entries.back().value;
      }
    }
    return *output_;
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
    if (frame.kind == FrameKind::Sequence) {
      if (frame.written >= frame.expected) {
        return fail("too many sequence elements were written");
      }
      ++frame.written;
      return true;
    }
    if (frame.kind == FrameKind::BufferedSequence) {
      if (frame.written >= frame.expected) {
        return fail("too many set elements were written");
      }
      frame.entries.push_back(Entry{{}, {}});
      ++frame.written;
      return true;
    }
    if (!frame.expecting_value) {
      return fail("object value was written without a field name");
    }
    frame.expecting_value = false;
    return true;
  }

  bool scalar_byte(std::uint8_t value) {
    if (!before_value()) {
      return false;
    }
    active_output().push_back(static_cast<char>(value));
    return true;
  }

  bool begin_object(std::size_t size, FrameKind kind, bool compact) {
    if (!before_value()) {
      return false;
    }
    if (kind == FrameKind::OrderedObject) {
      append_head(active_output(), 5, size);
    }
    frames_.emplace_back(kind, size, compact);
    if (kind == FrameKind::BufferedObject) {
      frames_.back().entries.reserve(size);
    }
    return true;
  }

  bool write_key(std::string key) {
    if (frames_.empty() || is_sequence_frame(frames_.back().kind)) {
      return fail("field name was written outside an object");
    }
    Frame &frame = frames_.back();
    if (frame.expecting_value) {
      return fail("object field is missing its value");
    }
    if (frame.written >= frame.expected) {
      return fail("too many object fields were written");
    }
    ++frame.written;
    if (frame.kind == FrameKind::BufferedObject) {
      frame.entries.push_back(Entry{std::move(key), {}});
    } else {
      active_output() += key;
    }
    frame.expecting_value = true;
    return true;
  }
};

} // namespace serdde_wire
