#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct BenchRecord {
    std::uint64_t id;
    std::string name;
    bool active;
    std::vector<std::int32_t> scores;
    std::map<std::string, std::string> metadata;
};

static nlohmann::json encode(const BenchRecord& value) {
    return {
        {"id", value.id},
        {"name", value.name},
        {"active", value.active},
        {"scores", value.scores},
        {"metadata", value.metadata},
    };
}

static BenchRecord decode(const nlohmann::json& value) {
    return BenchRecord{
        value.at("id").get<std::uint64_t>(),
        value.at("name").get<std::string>(),
        value.at("active").get<bool>(),
        value.at("scores").get<std::vector<std::int32_t>>(),
        value.at("metadata").get<std::map<std::string, std::string>>(),
    };
}

static double elapsed_ms(
    std::chrono::steady_clock::time_point start,
    std::chrono::steady_clock::time_point finish) {
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

int main() {
    constexpr std::int32_t iterations = 50000;
    const BenchRecord record{
        7,
        "Ada Lovelace",
        true,
        {2, 3, 5, 7, 11, 13, 17, 19},
        {{"format", "json"}, {"language", "dudu"}},
    };
    const std::string source = encode(record).dump();

    std::size_t bytes = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::int32_t i = 0; i < iterations; ++i) {
        bytes += encode(record).dump().size();
    }
    const auto serialize_end = std::chrono::steady_clock::now();

    std::uint64_t id_sum = 0;
    for (std::int32_t i = 0; i < iterations; ++i) {
        id_sum += decode(nlohmann::json::parse(source)).id;
    }
    const auto parse_end = std::chrono::steady_clock::now();

    std::cout << "implementation=nlohmann\n"
              << "iterations=" << iterations << '\n'
              << "serialize_ms=" << elapsed_ms(start, serialize_end) << '\n'
              << "parse_ms=" << elapsed_ms(serialize_end, parse_end) << '\n'
              << "bytes=" << bytes << '\n'
              << "id_sum=" << id_sum << '\n';
}
