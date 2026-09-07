#pragma once

#include <string>
#include <cstdint>
#include <compare>

struct MetricPoint {
    std::string name;
    double value = 0.0;
    std::int64_t timestamp = 0;

    auto operator<=>(const MetricPoint&) const = default;
};