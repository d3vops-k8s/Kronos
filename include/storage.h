#pragma once 

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cstddef>
#include "metric.h"
#include "ring_buffer.h"

class InMemoryStorage {
public:
    explicit InMemoryStorage(std::size_t default_capacity = 1000);

    void insert(const MetricPoint& point);

    std::optional<MetricPoint> get_latest(const std::string& metric_name) const;

    std::size_t metric_count() const;

    bool has_metric(const std::string& metric_name) const;

private:
    std::size_t default_capacity_;

    std::unordered_map<std::string, std::unique_ptr<RingBuffer>> storage_;
};