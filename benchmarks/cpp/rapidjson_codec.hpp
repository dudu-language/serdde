#pragma once

#include "benchmark_schemas.hpp"

#include <string>

namespace serdde_benchmark {

bool rapidjson_encode(const SmallRecord &value, std::string &output);
bool rapidjson_encode(const NestedRecord &value, std::string &output);
bool rapidjson_encode(const StringRecord &value, std::string &output);
bool rapidjson_encode(const ArrayRecord &value, std::string &output);
bool rapidjson_encode(const MapRecord &value, std::string &output);
bool rapidjson_encode(const EventRecord &value, std::string &output);

bool rapidjson_decode(const std::string &source, SmallRecord &output);
bool rapidjson_decode(const std::string &source, NestedRecord &output);
bool rapidjson_decode(const std::string &source, StringRecord &output);
bool rapidjson_decode(const std::string &source, ArrayRecord &output);
bool rapidjson_decode(const std::string &source, MapRecord &output);
bool rapidjson_decode(const std::string &source, EventRecord &output);

} // namespace serdde_benchmark
