#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace serdde_cbor_test {

inline char hex_digit(unsigned value) {
  return static_cast<char>(value < 10 ? '0' + value : 'a' + value - 10);
}

inline std::string hex(const std::string &value) {
  std::string result;
  result.reserve(value.size() * 2);
  for (const unsigned char byte : value) {
    result.push_back(hex_digit(byte >> 4));
    result.push_back(hex_digit(byte & 0x0f));
  }
  return result;
}

inline unsigned hex_value(char value) {
  if (value >= '0' && value <= '9') {
    return static_cast<unsigned>(value - '0');
  }
  return static_cast<unsigned>(value - 'a' + 10);
}

inline std::string bytes(std::string_view hex) {
  std::string result;
  result.reserve(hex.size() / 2);
  for (std::size_t index = 0; index + 1 < hex.size(); index += 2) {
    result.push_back(static_cast<char>((hex_value(hex[index]) << 4) |
                                      hex_value(hex[index + 1])));
  }
  return result;
}

} // namespace serdde_cbor_test
