#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include <cstdint>
#include "metric.h"

// Fixed-capacity circular buffer for storing a single metric's time series.
//
// Optimized for zero heap allocations during push(): stores lightweight Sample structs (16 bytes).
// When the buffer is full, the oldest entry is silently overwritten (FIFO eviction).
// All operations are O(1) except average() and get_all() which are O(N).
//
// Thread safety: None. Access must be externally synchronized.
// For concurrent use, see ThreadSafeStorage.
class RingBuffer {
public:
    // Capacity is clamped to a minimum of 1 to prevent modulo-by-zero in push().
    explicit RingBuffer(std::size_t capacity);

    // Writes sample at head_ and advances head_ with wrap-around.
    // Overwrites the oldest entry when the buffer is full.
    void push(const Sample& sample);
    void push(std::int64_t timestamp, double value);

    // Returns the most recently pushed entry, or nullopt if the buffer is empty.
    std::optional<Sample> get_latest() const;

    // Returns the arithmetic mean of all stored values, or nullopt if empty.
    std::optional<double> average() const;

    // Returns all stored samples in chronological order (oldest to newest).
    std::vector<Sample> get_all() const;

    // Returns samples filtered by [start, end] epoch seconds, optionally limited to `limit` entries.
    std::vector<Sample> get_range(std::int64_t start_time = 0, std::int64_t end_time = 0, std::size_t limit = 0) const;

    std::size_t size()     const;  // Number of valid entries currently stored.
    std::size_t capacity() const;  // Maximum number of entries the buffer can hold.
    bool        empty()    const;

private:
    std::vector<Sample> buffer_;
    std::size_t         capacity_;
    std::size_t         head_  = 0;  // Index of the next write position.
    std::size_t         count_ = 0;  // Number of valid entries (saturates at capacity_).
};