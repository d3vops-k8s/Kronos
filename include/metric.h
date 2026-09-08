#pragma once

#include <string>
#include <cstdint>
#include <compare>

// A single time-series data point.
// Mirrors the Prometheus exposition format:
//   <metric_name> <value> [<unix_timestamp_seconds>]
struct MetricPoint {
    std::string  name;
    double       value     = 0.0;
    std::int64_t timestamp = 0;  // Unix epoch seconds; 0 means "not provided"

    // Generates all six comparison operators (==, !=, <, <=, >, >=).
    // partial_ordering is required because double does not satisfy strict weak ordering
    // due to NaN (NaN != NaN violates reflexivity of equality).
    std::partial_ordering operator<=>(const MetricPoint&) const = default;
};