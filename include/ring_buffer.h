#pragma once 

#include <vector>
#include <optional>
#include <cstddef>
#include "metric.h"

class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity);

    void push(const MetricPoint& point);

    std::optional<MetricPoint> get_latest() const;
    std::optional<double> average() const;
    
    std::size_t size() const;
    std::size_t capacity() const;
    bool empty() const;
    
private:
    std::vector<MetricPoint> buffer_;
    std::size_t capacity_;
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};