#pragma once

#include "cbor_wire_common.hpp"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace serdde_wire {

namespace cbor_detail {

struct Document {
  std::string_view source;
  Node root;
  Error error;

  explicit Document(const std::string &input) : source(input) {
    if (!parse_node(source, 0, 0, root, error, true)) {
      root = Node{};
      return;
    }
    if (root.end != source.size()) {
      fail(error, root.end, "trailing data after CBOR root value");
      root = Node{};
    }
  }

  bool valid() const { return error.message.empty(); }
};

} // namespace cbor_detail

class CborReader {
public:
  CborReader() = default;

  explicit CborReader(const std::string &source)
      : document_(std::make_shared<cbor_detail::Document>(source)) {
    if (document_->valid()) {
      node_ = document_->root;
      valid_ = true;
    }
  }

  bool valid() const { return valid_; }
  std::string format_name() const { return "cbor"; }
  std::string error_message() const {
    if (document_ == nullptr) {
      return "invalid CBOR reader";
    }
    return document_->error.message;
  }
  std::size_t line() const { return 1; }
  std::size_t column() const { return offset() + 1; }
  std::size_t offset() const {
    if (document_ == nullptr) {
      return 0;
    }
    return valid_ ? node_.begin : document_->error.offset;
  }

  bool is_null() const {
    return valid_ && node_.major == 7 && node_.additional == 22;
  }
  bool is_unsigned_integer() const { return valid_ && node_.major == 0; }
  bool is_negative_integer() const { return valid_ && node_.major == 1; }
  bool is_float() const {
    return valid_ && node_.major == 7 &&
           (node_.additional == 25 || node_.additional == 26 ||
            node_.additional == 27);
  }
  bool is_sequence() const { return valid_ && node_.major == 4; }
  bool is_object() const { return valid_ && node_.major == 5; }

  bool get_bool(bool &output) const {
    if (!valid_ || node_.major != 7 ||
        (node_.additional != 20 && node_.additional != 21)) {
      return false;
    }
    output = node_.additional == 21;
    return true;
  }

  bool get_i64(std::int64_t &output) const {
    if (!valid_) {
      return false;
    }
    if (node_.major == 0 &&
        node_.argument <=
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      output = static_cast<std::int64_t>(node_.argument);
      return true;
    }
    if (node_.major == 1 &&
        node_.argument <=
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      output = -1 - static_cast<std::int64_t>(node_.argument);
      return true;
    }
    return false;
  }

  bool get_u64(std::uint64_t &output) const {
    if (!valid_ || node_.major != 0) {
      return false;
    }
    output = node_.argument;
    return true;
  }

  bool get_f64(double &output) const {
    if (!valid_ || node_.major != 7) {
      return false;
    }
    if (node_.additional == 25) {
      output = cbor_detail::half_to_double(
          static_cast<std::uint16_t>(node_.argument));
    } else if (node_.additional == 26) {
      output = static_cast<double>(
          std::bit_cast<float>(static_cast<std::uint32_t>(node_.argument)));
    } else if (node_.additional == 27) {
      output = std::bit_cast<double>(node_.argument);
    } else {
      return false;
    }
    return std::isfinite(output);
  }

  bool get_string(std::string &output) const {
    if (!valid_ || node_.major != 3) {
      return false;
    }
    output.assign(document_->source.data() + node_.payload,
                  static_cast<std::size_t>(node_.argument));
    return true;
  }

  std::size_t size() const {
    if (!valid_ || (node_.major != 4 && node_.major != 5)) {
      return 0;
    }
    return static_cast<std::size_t>(node_.argument);
  }

  std::string key(std::size_t index) const {
    cbor_detail::Node key_node;
    cbor_detail::Node value_node;
    if (!map_pair(index, key_node, value_node)) {
      return "";
    }
    return key_label(key_node);
  }

  CborReader element(std::size_t index) const {
    if (!valid_) {
      return CborReader();
    }
    cbor_detail::Node child;
    if (node_.major == 4 && sequence_element(index, child)) {
      return CborReader(document_, child);
    }
    cbor_detail::Node key_node;
    if (node_.major == 5 && map_pair(index, key_node, child)) {
      return CborReader(document_, child);
    }
    return CborReader();
  }

  bool has_field(const std::string &name) const {
    return field_count(name) != 0;
  }

  std::size_t field_count(const std::string &name) const {
    std::size_t count = 0;
    visit_fields([&](const cbor_detail::Node &key_node,
                     const cbor_detail::Node &) {
      if (text_key_equals(key_node, name)) {
        ++count;
      }
    });
    return count;
  }

  CborReader field(const std::string &name) const {
    return find_field(name, 0, false);
  }

  bool has_field_id(const std::string &name, std::uint64_t id) const {
    return field_count_id(name, id) != 0;
  }

  std::size_t field_count_id(const std::string &name, std::uint64_t id) const {
    std::size_t count = 0;
    visit_fields([&](const cbor_detail::Node &key_node,
                     const cbor_detail::Node &) {
      if ((key_node.major == 0 && key_node.argument == id) ||
          text_key_equals(key_node, name)) {
        ++count;
      }
    });
    return count;
  }

  CborReader field_id(const std::string &name, std::uint64_t id) const {
    return find_field(name, id, true);
  }

  bool key_matches(std::size_t index, const std::string &name,
                   std::uint64_t id) const {
    cbor_detail::Node key_node;
    cbor_detail::Node value_node;
    if (!map_pair(index, key_node, value_node)) {
      return false;
    }
    return (key_node.major == 0 && key_node.argument == id) ||
           text_key_equals(key_node, name);
  }

private:
  std::shared_ptr<cbor_detail::Document> document_;
  cbor_detail::Node node_;
  bool valid_ = false;

  CborReader(std::shared_ptr<cbor_detail::Document> document,
             cbor_detail::Node node)
      : document_(std::move(document)), node_(node), valid_(true) {}

  bool read_node(std::size_t offset, cbor_detail::Node &node) const {
    cbor_detail::Error ignored;
    return cbor_detail::parse_node(document_->source, offset, 0, node, ignored,
                                   false);
  }

  bool sequence_element(std::size_t index, cbor_detail::Node &output) const {
    if (node_.major != 4 || index >= node_.argument) {
      return false;
    }
    std::size_t cursor = node_.payload;
    for (std::size_t current = 0; current <= index; ++current) {
      if (!read_node(cursor, output)) {
        return false;
      }
      cursor = output.end;
    }
    return true;
  }

  bool map_pair(std::size_t index, cbor_detail::Node &key_node,
                cbor_detail::Node &value_node) const {
    if (node_.major != 5 || index >= node_.argument) {
      return false;
    }
    std::size_t cursor = node_.payload;
    for (std::size_t current = 0; current <= index; ++current) {
      if (!read_node(cursor, key_node)) {
        return false;
      }
      cursor = key_node.end;
      if (!read_node(cursor, value_node)) {
        return false;
      }
      cursor = value_node.end;
    }
    return true;
  }

  template <typename Visitor> void visit_fields(Visitor visitor) const {
    if (!valid_ || node_.major != 5) {
      return;
    }
    std::size_t cursor = node_.payload;
    for (std::uint64_t index = 0; index < node_.argument; ++index) {
      cbor_detail::Node key_node;
      cbor_detail::Node value_node;
      if (!read_node(cursor, key_node)) {
        return;
      }
      cursor = key_node.end;
      if (!read_node(cursor, value_node)) {
        return;
      }
      cursor = value_node.end;
      visitor(key_node, value_node);
    }
  }

  bool text_key_equals(const cbor_detail::Node &key_node,
                       std::string_view name) const {
    return key_node.major == 3 && key_node.argument == name.size() &&
           document_->source.substr(
               key_node.payload, static_cast<std::size_t>(key_node.argument)) ==
               name;
  }

  std::string key_label(const cbor_detail::Node &key_node) const {
    if (key_node.major == 0) {
      return "#" + std::to_string(key_node.argument);
    }
    if (key_node.major == 3) {
      return std::string(document_->source.data() + key_node.payload,
                         static_cast<std::size_t>(key_node.argument));
    }
    return "<unsupported key>";
  }

  CborReader find_field(std::string_view name, std::uint64_t id,
                        bool include_id) const {
    CborReader result;
    visit_fields([&](const cbor_detail::Node &key_node,
                     const cbor_detail::Node &value_node) {
      if (!result.valid_ &&
          (text_key_equals(key_node, name) ||
           (include_id && key_node.major == 0 && key_node.argument == id))) {
        result = CborReader(document_, value_node);
      }
    });
    return result;
  }
};

} // namespace serdde_wire
