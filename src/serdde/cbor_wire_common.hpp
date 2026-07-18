#pragma once

#include "json_native.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace serdde_wire::cbor_detail {

struct Error {
  std::string message;
  std::size_t offset = 0;
};

struct Node {
  std::size_t begin = 0;
  std::size_t payload = 0;
  std::size_t end = 0;
  std::uint8_t major = 0xff;
  std::uint8_t additional = 0;
  std::uint64_t argument = 0;
};

inline bool fail(Error &error, std::size_t offset, std::string message) {
  if (error.message.empty()) {
    error.offset = offset;
    error.message = std::move(message);
  }
  return false;
}

inline bool read_unsigned(std::string_view source, std::size_t &cursor,
                          std::size_t width, std::uint64_t &value,
                          Error &error) {
  if (width > source.size() - std::min(cursor, source.size())) {
    return fail(error, cursor, "truncated CBOR argument");
  }
  value = 0;
  for (std::size_t index = 0; index < width; ++index) {
    value = (value << 8) |
            static_cast<unsigned char>(source[cursor + index]);
  }
  cursor += width;
  return true;
}

inline bool read_head(std::string_view source, std::size_t offset, Node &node,
                      Error &error) {
  if (offset >= source.size()) {
    return fail(error, offset, "unexpected end of CBOR input");
  }
  node = Node{};
  node.begin = offset;
  const auto initial = static_cast<unsigned char>(source[offset]);
  node.major = static_cast<std::uint8_t>(initial >> 5);
  node.additional = static_cast<std::uint8_t>(initial & 0x1f);
  std::size_t cursor = offset + 1;
  if (node.additional <= 23) {
    node.argument = node.additional;
  } else if (node.additional == 24) {
    if (!read_unsigned(source, cursor, 1, node.argument, error)) {
      return false;
    }
  } else if (node.additional == 25) {
    if (!read_unsigned(source, cursor, 2, node.argument, error)) {
      return false;
    }
  } else if (node.additional == 26) {
    if (!read_unsigned(source, cursor, 4, node.argument, error)) {
      return false;
    }
  } else if (node.additional == 27) {
    if (!read_unsigned(source, cursor, 8, node.argument, error)) {
      return false;
    }
  } else if (node.additional == 31) {
    return fail(error, offset,
                "indefinite-length CBOR values are not supported");
  } else {
    return fail(error, offset, "reserved CBOR additional information");
  }
  node.payload = cursor;
  return true;
}

inline bool parse_node(std::string_view source, std::size_t offset,
                       std::size_t depth, Node &node, Error &error,
                       bool validate);

inline bool same_key(std::string_view source, const Node &left,
                     const Node &right) {
  if (left.major != right.major) {
    return false;
  }
  if (left.major == 0) {
    return left.argument == right.argument;
  }
  if (left.major != 3 || left.argument != right.argument) {
    return false;
  }
  return source.substr(left.payload, static_cast<std::size_t>(left.argument)) ==
         source.substr(right.payload, static_cast<std::size_t>(right.argument));
}

inline bool parse_container(std::string_view source, std::size_t depth,
                            Node &node, Error &error, bool validate) {
  std::size_t cursor = node.payload;
  if (node.argument > source.size()) {
    return fail(error, node.begin, "CBOR container length is too large");
  }
  std::vector<Node> keys;
  if (validate && node.major == 5) {
    keys.reserve(static_cast<std::size_t>(node.argument));
  }
  for (std::uint64_t index = 0; index < node.argument; ++index) {
    Node child;
    if (!parse_node(source, cursor, depth + 1, child, error, validate)) {
      return false;
    }
    cursor = child.end;
    if (node.major != 5) {
      continue;
    }
    if (child.major != 0 && child.major != 3) {
      return fail(error, child.begin,
                  "CBOR object keys must be strings or unsigned integers");
    }
    if (validate) {
      for (const Node &previous : keys) {
        if (same_key(source, previous, child)) {
          return fail(error, child.begin, "duplicate CBOR object key");
        }
      }
      keys.push_back(child);
    }
    if (!parse_node(source, cursor, depth + 1, child, error, validate)) {
      return false;
    }
    cursor = child.end;
  }
  node.end = cursor;
  return true;
}

inline bool parse_node(std::string_view source, std::size_t offset,
                       std::size_t depth, Node &node, Error &error,
                       bool validate) {
  if (depth > 128) {
    return fail(error, offset, "CBOR nesting depth exceeds 128");
  }
  if (!read_head(source, offset, node, error)) {
    return false;
  }
  if (node.major <= 1) {
    node.end = node.payload;
    return true;
  }
  if (node.major == 2 || node.major == 3) {
    if (node.argument > source.size() -
                            std::min(node.payload, source.size())) {
      return fail(error, node.begin, "truncated CBOR string");
    }
    node.end = node.payload + static_cast<std::size_t>(node.argument);
    if (node.major == 3 &&
        !serdde_json_native::valid_utf8(std::string_view(
            source.data() + node.payload,
            static_cast<std::size_t>(node.argument)))) {
      return fail(error, node.payload, "CBOR text is not valid UTF-8");
    }
    return true;
  }
  if (node.major == 4 || node.major == 5) {
    return parse_container(source, depth, node, error, validate);
  }
  if (node.major == 6) {
    return fail(error, node.begin, "CBOR tags are not supported");
  }
  if (node.major != 7) {
    return fail(error, node.begin, "unknown CBOR major type");
  }
  if (node.additional == 20 || node.additional == 21 ||
      node.additional == 22 || node.additional == 25 ||
      node.additional == 26 || node.additional == 27) {
    node.end = node.payload;
    return true;
  }
  return fail(error, node.begin, "unsupported CBOR simple value");
}

inline double half_to_double(std::uint16_t bits) {
  const bool negative = (bits & 0x8000u) != 0;
  const std::uint16_t exponent = (bits >> 10) & 0x1fu;
  const std::uint16_t fraction = bits & 0x03ffu;
  double value = 0.0;
  if (exponent == 0) {
    value = std::ldexp(static_cast<double>(fraction), -24);
  } else if (exponent == 31) {
    value = fraction == 0 ? std::numeric_limits<double>::infinity()
                          : std::numeric_limits<double>::quiet_NaN();
  } else {
    value = std::ldexp(static_cast<double>(fraction + 1024),
                       static_cast<int>(exponent) - 25);
  }
  return negative ? -value : value;
}

} // namespace serdde_wire::cbor_detail
