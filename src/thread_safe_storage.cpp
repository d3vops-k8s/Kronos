#include "thread_safe_storage.h"
#include <mutex>  // std::unique_lock

ThreadSafeStorage::ThreadSafeStorage(std::size_t default_capacity)
    : storage_(default_capacity) {}

void ThreadSafeStorage::insert(const MetricPoint& point) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    storage_.insert(point);
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
