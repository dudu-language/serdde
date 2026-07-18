#pragma once

#include <limits>
#include <string>

namespace serdde_json_test {

inline double infinity() {
    return std::numeric_limits<double>::infinity();
}

inline std::string invalid_utf8() {
    return std::string{"\xc0\x80", 2};
}

} // namespace serdde_json_test
