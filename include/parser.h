#pragma once

#include <string_view>
#include <optional>
#include "metric.h"

std::optional<MetricPoint> parse_line(std::string_view line);