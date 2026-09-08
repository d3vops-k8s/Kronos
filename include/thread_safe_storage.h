#pragma once

#include <shared_mutex>
#include <string>
#include <vector>
#include <optional>
#include <cstddef>
#include "metric.h"
#include "storage.h"

// Thread-safe wrapper around InMemoryStorage using a readers-writer lock.
//
// Concurrency model:
//   - Multiple reader threads may hold a shared_lock simultaneously (no blocking).
//   - A single writer thread acquires a unique_lock, blocking all other readers and writers
//     for the duration of the write.
//
// All locks are RAII-managed (std::shared_lock / std::unique_lock).
// They are released automatically on scope exit, including during stack unwinding.
class ThreadSafeStorage {
public:
    explicit ThreadSafeStorage(std::size_t default_capacity = 1000);

    // Acquires an exclusive lock. Blocks until all active readers have released.
    void insert(const MetricPoint& point);

    // Acquire shared locks. Multiple callers may execute these concurrently.
    std::optional<MetricPoint> get_latest(const std::string& metric_name) const;
    std::optional<double>      get_average(const std::string& metric_name) const;
    std::size_t                metric_count() const;

     // Returns all points for a metric under shared_lock (thread-safe read).
    std::vector<MetricPoint> get_points(const std::string& metric_name) const;
    
    bool                       has_metric(const std::string& metric_name) const;

    // Returns a snapshot copy of metric names. The lock is held only during
    // the copy construction, not for the lifetime of the returned vector.
    std::vector<std::string>   metric_names() const;

private:
    InMemoryStorage storage_;

    // mutable: permits locking inside const member functions.
    // Logically the mutex is not part of the object's observable state.
    mutable std::shared_mutex mutex_;
};