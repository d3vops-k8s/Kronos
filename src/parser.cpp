#include "parser.h"
#include <charconv>
#include <cmath>
#include <string_view>

std::optional<MetricPoint> parse_line(std::string_view line) {
    // Skip empty lines and OpenMetrics comment / directive lines (# HELP, # TYPE).
    if (line.empty() || line[0] == '#') {
        return std::nullopt;
    }

    auto space1 = line.find(' ');
    if (space1 == std::string_view::npos) {
        return std::nullopt;
    }

    MetricPoint point;
    point.name = std::string(line.substr(0, space1));

    auto value_start = line.data() + space1 + 1;
    auto line_end    = line.data() + line.size();

    // Consume optional whitespace between name and value.
    while (value_start < line_end && (*value_start == ' ' || *value_start == '\t')) {
        ++value_start;
    }

    // std::from_chars: zero-allocation, locale-independent, noexcept numeric parsing.
    // Preferred over std::stod which allocates and performs locale lookup.
    auto [ptr, ec] = std::from_chars(value_start, line_end, point.value);
    if (ec != std::errc{}) {
        return std::nullopt;
    }

    // Reject NaN and Inf — they are valid IEEE 754 but meaningless in a TSDB context.
    if (!std::isfinite(point.value)) {
        return std::nullopt;
    }

    // Parse optional timestamp. Consume whitespace first.
    while (ptr < line_end && (*ptr == ' ' || *ptr == '\t')) {
        ++ptr;
    }
    if (ptr < line_end) {
        std::from_chars(ptr, line_end, point.timestamp);
    }

    return point;
}