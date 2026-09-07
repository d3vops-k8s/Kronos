#include "storage.h"
#include "metric.h"
#include "ring_buffer.h"
#include <cstddef>
#include <memory>

InMemoryStorage::InMemoryStorage(std::size_t default_capacity)
    : default_capacity_(default_capacity) {}

void InMemoryStorage::insert(const MetricPoint& point) {
    if (storage_.find(point.name) == storage_.end()) {
        storage_[point.name] = std::make_unique<RingBuffer>(default_capacity_);
    }

    storage_[point.name]->push(point);
}

std::optional<MetricPoint> InMemoryStorage::get_latest(const std::string& metric_name) const {
    auto it = storage_.find(metric_name);

    if (it == storage_.end()) {
        return std::nullopt;
    }

    return it->second->get_latest();
}

std::size_t InMemoryStorage::metric_count() const {
    return storage_.size();
}

bool InMemoryStorage::has_metric(const std::string& metric_name) const {
    return storage_.contains(metric_name);
}