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
    it->second->push(point);
}

std::optional<MetricPoint> InMemoryStorage::get_latest(const std::string& metric_name) const {
    auto it = storage_.find(metric_name);
    if (it == storage_.end()) {
        return std::nullopt;
    }
    return it->second->get_latest();
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
    auto it = storage_.find(metric_name);
    if (it == storage_.end()) {
        return {};
    }
    return it->second->get_all();
}