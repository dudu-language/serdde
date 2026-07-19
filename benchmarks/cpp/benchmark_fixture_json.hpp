#pragma once

#include "benchmark_schemas.hpp"

#include <string>

namespace serdde_benchmark {

bool benchmark_json(const SmallRecord &value, std::string &output);
bool benchmark_json(const NestedRecord &value, std::string &output);
bool benchmark_json(const StringRecord &value, std::string &output);
bool benchmark_json(const ArrayRecord &value, std::string &output);
bool benchmark_json(const MapRecord &value, std::string &output);
bool benchmark_json(const EventRecord &value, std::string &output);

} // namespace serdde_benchmark
