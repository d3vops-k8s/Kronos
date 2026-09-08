#pragma once

#include <shared_mutex>
#include <string>
#include <vector>
#include <cstddef>
#include "metric.h"
#include "storage.h"

class ThreadSafeStorage {
public:

    explicit ThreadSafeStorage(std::size_t default_capacity = 1000);

    void insert(const MetricPoint& point);

    std::optional<MetricPoint> get_latest(const std::string& metric_name) const;
    std::optional<double> get_average(const std::string& metric_name) const;
    std::size_t metric_count() const;
    bool has_metric(const std::string& metric_name) const;
    std::vector<std::string> metric_names() const;

private:

    InMemoryStorage storage_;
    mutable std::shared_mutex mutex_;
};