#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace serdde_json_native {

inline bool parse_i64(const std::string& text, std::int64_t* output) {
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [cursor, error] = std::from_chars(begin, end, *output, 10);
    return error == std::errc{} && cursor == end;
}

inline bool parse_u64(const std::string& text, std::uint64_t* output) {
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [cursor, error] = std::from_chars(begin, end, *output, 10);
    return error == std::errc{} && cursor == end;
}

inline bool parse_f64(const std::string& text, double* output) {
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [cursor, error] =
        std::from_chars(begin, end, *output, std::chars_format::general);
    return error == std::errc{} && cursor == end && std::isfinite(*output);
}

inline bool finite_f64(double value) {
    return std::isfinite(value);
}

inline std::string format_f64(double value) {
    char buffer[128]{};
    const auto [cursor, error] = std::to_chars(
        buffer,
        buffer + sizeof(buffer),
        value,
        std::chars_format::general,
        std::numeric_limits<double>::max_digits10);
    if (error != std::errc{}) {
        return {};
    }
    std::string output(buffer, cursor);
    if (output.find_first_of(".eE") == std::string::npos) {
        output += ".0";
    }
    return output;
}

inline bool valid_utf8(const std::string& text) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    std::size_t index = 0;
    while (index < text.size()) {
        const unsigned char lead = bytes[index++];
        if (lead <= 0x7f) {
            continue;
        }

        std::uint32_t codepoint = 0;
        std::size_t continuation_count = 0;
        std::uint32_t minimum = 0;
        if (lead >= 0xc2 && lead <= 0xdf) {
            codepoint = lead & 0x1f;
            continuation_count = 1;
            minimum = 0x80;
        } else if (lead >= 0xe0 && lead <= 0xef) {
            codepoint = lead & 0x0f;
            continuation_count = 2;
            minimum = 0x800;
        } else if (lead >= 0xf0 && lead <= 0xf4) {
            codepoint = lead & 0x07;
            continuation_count = 3;
            minimum = 0x10000;
        } else {
            return false;
        }

        if (index + continuation_count > text.size()) {
            return false;
        }
        for (std::size_t i = 0; i < continuation_count; ++i) {
            const unsigned char next = bytes[index++];
            if ((next & 0xc0) != 0x80) {
                return false;
            }
            codepoint = (codepoint << 6) | (next & 0x3f);
        }
        if (codepoint < minimum || codepoint > 0x10ffff ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff)) {
            return false;
        }
    }
    return true;
}

} // namespace serdde_json_native
