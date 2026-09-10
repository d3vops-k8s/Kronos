#include "thread_safe_storage.h"
#include <mutex>  // std::unique_lock
#include "wal.h"

ThreadSafeStorage::ThreadSafeStorage(std::size_t default_capacity)
    : storage_(default_capacity) {}

void ThreadSafeStorage::insert(const MetricPoint& point) {
    if (wal_) {
        wal_->append(point);
    }
    std::unique_lock<std::shared_mutex> lock(mutex_);
    storage_.insert(point);
    total_samples_ingested_.fetch_add(1, std::memory_order_relaxed);
}

void ThreadSafeStorage::insert_batch(std::span<const MetricPoint> points) {
    if (points.empty()) {
        return;
    }
    if (wal_) {
        wal_->append_batch(points);
    }
    std::unique_lock<std::shared_mutex> lock(mutex_);
    storage_.insert_batch(points);
    total_samples_ingested_.fetch_add(points.size(), std::memory_order_relaxed);
}

std::optional<MetricPoint> ThreadSafeStorage::get_latest(const std::string& metric_name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return storage_.get_latest(metric_name);
}

std::optional<double> ThreadSafeStorage::get_average(const std::string& metric_name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return storage_.get_average(metric_name);
}

std::size_t ThreadSafeStorage::metric_count() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return storage_.metric_count();
}

bool ThreadSafeStorage::has_metric(const std::string& metric_name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return storage_.has_metric(metric_name);
}

std::vector<std::string> ThreadSafeStorage::metric_names() const {
    // Lock is released as soon as the vector copy is constructed.
    // Callers receive a private copy and access it without holding the lock.
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return storage_.metric_names();
}

std::vector<MetricPoint> ThreadSafeStorage::get_points(const std::string& metric_name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return storage_.get_points(metric_name);
}

std::vector<MetricPoint> ThreadSafeStorage::get_points_range(
    const std::string& metric_name,
    std::int64_t start_time,
    std::int64_t end_time,
    std::size_t limit
) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return storage_.get_points_range(metric_name, start_time, end_time, limit);
}

std::uint64_t ThreadSafeStorage::total_samples_ingested() const {
    return total_samples_ingested_.load(std::memory_order_relaxed);
}

void ThreadSafeStorage::set_wal(wal::WALWriter* wal) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    wal_ = wal;
}