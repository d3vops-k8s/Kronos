#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include "metric.h"

// Fixed-capacity circular buffer for storing a single metric's time series.
//
// When the buffer is full, the oldest entry is silently overwritten (FIFO eviction).
// All operations are O(1) except average() which is O(N).
//
// Memory layout is a contiguous std::vector allocated once at construction.
// No further heap allocation occurs during push().
//
// Thread safety: None. Access must be externally synchronized.
// For concurrent use, see ThreadSafeStorage.
class RingBuffer {
public:
    // Capacity is clamped to a minimum of 1 to prevent modulo-by-zero in push().
    explicit RingBuffer(std::size_t capacity);

    // Writes point at head_ and advances head_ with wrap-around.
    // Overwrites the oldest entry when the buffer is full.
    void push(const MetricPoint& point);

    // Returns the most recently pushed entry, or nullopt if the buffer is empty.
    // Returns by value; a reference would be invalidated by the next push().
    std::optional<MetricPoint> get_latest() const;

    // Returns the arithmetic mean of all stored values, or nullopt if empty.
    // Iteration order is chronological (oldest to newest).
    std::optional<double> average() const;
    

    // Returns all stored data points in chronological order (oldest to newest).
    std::vector<MetricPoint> get_all() const;

    std::size_t size()     const;  // Number of valid entries currently stored.
    std::size_t capacity() const;  // Maximum number of entries the buffer can hold.
    bool        empty()    const;

private:
    std::vector<MetricPoint> buffer_;
    std::size_t              capacity_;
    std::size_t              head_  = 0;  // Index of the next write position.
    std::size_t              count_ = 0;  // Number of valid entries (saturates at capacity_).
};