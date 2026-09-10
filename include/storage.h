#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include <span>
#include <cstddef>
#include "metric.h"
#include "ring_buffer.h"

// In-memory storage engine for time-series metrics.
//
// Each unique metric name is mapped to an independent RingBuffer that retains
// the last `default_capacity` data points. Older points are silently evicted.
//
// The map value is a heap-allocated RingBuffer held via unique_ptr to avoid
// copying the internal vector during unordered_map rehash operations.
//
// Thread safety: None. Use ThreadSafeStorage for concurrent access.
class InMemoryStorage {
public:
    // default_capacity: number of data points retained per metric.
    // At a 15-second scrape interval, 1000 points covers roughly 4 hours of history.
    explicit InMemoryStorage(std::size_t default_capacity = 1000);

    // Inserts a data point into the corresponding metric's RingBuffer.
    // Creates a new RingBuffer for the metric on first insertion.
    // Complexity: O(1) amortized.
    void insert(const MetricPoint& point);

    // Inserts a batch of data points into their respective RingBuffers.
    void insert_batch(std::span<const MetricPoint> points);

    // Returns the most recent data point for the given metric, or nullopt if not found.
    std::optional<MetricPoint> get_latest(const std::string& metric_name) const;

    // Returns the average value across all buffered points for the given metric.
    std::optional<double> get_average(const std::string& metric_name) const;

    // Returns all buffered points for the given metric in chronological order.
    std::vector<MetricPoint> get_points(const std::string& metric_name) const;

    // Returns true if the metric has been seen at least once.
    bool has_metric(const std::string& metric_name) const;

    // Returns a snapshot of all tracked metric names. Order is unspecified.
    // Intended for use by the HTTP API metrics list endpoint.
    std::vector<std::string> metric_names() const;

    // Returns the number of unique metrics currently tracked.
    std::size_t metric_count() const;

private:
    std::size_t default_capacity_;
    std::unordered_map<std::string, std::unique_ptr<RingBuffer>> storage_;
};