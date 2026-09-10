#include "storage.h"

InMemoryStorage::InMemoryStorage(std::size_t default_capacity)
    : default_capacity_(default_capacity) {}

void InMemoryStorage::insert(const MetricPoint& point) {
    // try_emplace performs a single hash lookup regardless of whether the key exists.
    // Equivalent naive code (find + operator[]) would perform up to three lookups.
    auto [it, inserted] = storage_.try_emplace(point.name);
    if (inserted) {
        it->second = std::make_unique<RingBuffer>(default_capacity_);
    }
    it->second->push(point.timestamp, point.value);
}

void InMemoryStorage::insert_batch(std::span<const MetricPoint> points) {
    for (const auto& point : points) {
        insert(point);
    }
}

std::optional<MetricPoint> InMemoryStorage::get_latest(const std::string& metric_name) const {
    auto it = storage_.find(metric_name);
    if (it == storage_.end()) {
        return std::nullopt;
    }
    auto sample = it->second->get_latest();
    if (!sample) {
        return std::nullopt;
    }
    return MetricPoint{metric_name, sample->value, sample->timestamp};
}

std::optional<double> InMemoryStorage::get_average(const std::string& metric_name) const {
    auto it = storage_.find(metric_name);
    if (it == storage_.end()) {
        return std::nullopt;
    }
    return it->second->average();
}

bool InMemoryStorage::has_metric(const std::string& metric_name) const {
    return storage_.contains(metric_name);
}

std::vector<std::string> InMemoryStorage::metric_names() const {
    std::vector<std::string> names;
    names.reserve(storage_.size());  // Avoid reallocation during iteration.
    for (const auto& [name, _] : storage_) {
        names.push_back(name);
    }
    return names;
}

std::size_t InMemoryStorage::metric_count() const {
    return storage_.size();
}

std::vector<MetricPoint> InMemoryStorage::get_points(const std::string& metric_name) const {
    return get_points_range(metric_name, 0, 0, 0);
}

std::vector<MetricPoint> InMemoryStorage::get_points_range(
    const std::string& metric_name,
    std::int64_t start_time,
    std::int64_t end_time,
    std::size_t limit
) const {
    auto it = storage_.find(metric_name);
    if (it == storage_.end()) {
        return {};
    }
    auto samples = it->second->get_range(start_time, end_time, limit);
    std::vector<MetricPoint> points;
    points.reserve(samples.size());
    for (const auto& s : samples) {
        points.push_back(MetricPoint{metric_name, s.value, s.timestamp});
    }
    return points;
}