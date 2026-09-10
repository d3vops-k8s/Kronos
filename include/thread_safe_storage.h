#pragma once

#include <shared_mutex>
#include <string>
#include <vector>
#include <optional>
#include <span>
#include <cstddef>
#include <cstdint>
#include <atomic>
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
namespace wal { class WALWriter; }
class ThreadSafeStorage {
public:
    explicit ThreadSafeStorage(std::size_t default_capacity = 1000);

    // Acquires an exclusive lock. Blocks until all active readers have released.
    void insert(const MetricPoint& point);

    // Acquires an exclusive lock once for the entire batch.
    void insert_batch(std::span<const MetricPoint> points);

    // Attaches a WAL writer. When set, every insert() is persisted to disk before RAM.
    void set_wal(wal::WALWriter* wal);

    // Acquire shared locks. Multiple callers may execute these concurrently.
    std::optional<MetricPoint> get_latest(const std::string& metric_name) const;
    std::optional<double>      get_average(const std::string& metric_name) const;
    std::size_t                metric_count() const;

    // Returns all points for a metric under shared_lock (thread-safe read).
    std::vector<MetricPoint> get_points(const std::string& metric_name) const;

    // Returns points filtered by time range and limit under shared_lock.
    std::vector<MetricPoint> get_points_range(
        const std::string& metric_name,
        std::int64_t start_time = 0,
        std::int64_t end_time = 0,
        std::size_t limit = 0
    ) const;

    bool has_metric(const std::string& metric_name) const;

    // Returns a snapshot copy of metric names. The lock is held only during
    // the copy construction, not for the lifetime of the returned vector.
    std::vector<std::string> metric_names() const;

    // Total metric samples ingested since startup (counter for self-monitoring).
    std::uint64_t total_samples_ingested() const;

private:
    InMemoryStorage storage_;

    // mutable: permits locking inside const member functions.
    // Logically the mutex is not part of the object's observable state.
    mutable std::shared_mutex mutex_;
    wal::WALWriter* wal_ = nullptr;
    std::atomic<std::uint64_t> total_samples_ingested_{0};
};