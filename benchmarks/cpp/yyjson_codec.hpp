#pragma once

#include "benchmark_schemas.hpp"

#include <string>

namespace serdde_benchmark {

bool yyjson_encode(const SmallRecord &value, std::string &output);
bool yyjson_encode(const NestedRecord &value, std::string &output);
bool yyjson_encode(const StringRecord &value, std::string &output);
bool yyjson_encode(const ArrayRecord &value, std::string &output);
bool yyjson_encode(const MapRecord &value, std::string &output);
bool yyjson_encode(const EventRecord &value, std::string &output);

bool yyjson_decode(const std::string &source, SmallRecord &output);
bool yyjson_decode(const std::string &source, NestedRecord &output);
bool yyjson_decode(const std::string &source, StringRecord &output);
bool yyjson_decode(const std::string &source, ArrayRecord &output);
bool yyjson_decode(const std::string &source, MapRecord &output);
bool yyjson_decode(const std::string &source, EventRecord &output);

} // namespace serdde_benchmark
