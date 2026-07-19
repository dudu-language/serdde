#pragma once

#include "benchmark_schemas.hpp"

#include <string>

namespace serdde_benchmark {

bool simdjson_decode(const std::string &source, SmallRecord &output);
bool simdjson_decode(const std::string &source, NestedRecord &output);
bool simdjson_decode(const std::string &source, StringRecord &output);
bool simdjson_decode(const std::string &source, ArrayRecord &output);
bool simdjson_decode(const std::string &source, MapRecord &output);
bool simdjson_decode(const std::string &source, EventRecord &output);

} // namespace serdde_benchmark
