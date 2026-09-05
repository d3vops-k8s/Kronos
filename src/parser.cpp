#include "parser.h"
#include "metric.h"
#include <optional>
#include <sstream>

std::optional<MetricPoint> parse_line(const std::string& line) {
    if (line.empty()) {
        return std::nullopt;
    }

    if (line[0] == '#') {
        return std::nullopt;
    }

    std::stringstream ss(line);
    MetricPoint point;

    if (!(ss >> point.name >> point.value)) {
        return std::nullopt;
    }

    std::int64_t ts = 0;
    if (ss >> ts) {
        point.timestamp = ts;
    }

    return point;
}