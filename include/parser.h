#pragma once

#include <string_view>
#include <optional>
#include "metric.h"

// Parses a single line from the Prometheus / OpenMetrics text exposition format.
//
// Expected format:
//   <metric_name> <value> [<unix_timestamp_seconds>]
//
// Returns nullopt for:
//   - Empty lines
//   - Comment lines starting with '#' (HELP, TYPE directives)
//   - Lines with malformed or non-finite (NaN, Inf) values
//
// The function accepts std::string_view to avoid heap allocation — the caller
// is responsible for ensuring the underlying buffer outlives the call.
std::optional<MetricPoint> parse_line(std::string_view line);