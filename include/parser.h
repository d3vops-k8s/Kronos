#pragma once 

#include <string>
#include <optional>
#include "metric.h"

std::optional<MetricPoint> parse_line(const std::string& line);