#include "ring_buffer.h"
#include "metric.h"
#include <cstddef>

RingBuffer::RingBuffer(std::size_t capacity)
    : capacity_(capacity) {
    buffer_.resize(capacity_);
    }

void RingBuffer::push(const MetricPoint& point) {
    buffer_[head_] = point;

    head_ = (head_ + 1) % capacity_;

    if (count_ < capacity_) {
        count_++;
    }
}

std::optional<MetricPoint> RingBuffer::get_latest() const {
    if (empty()) {
        return std::nullopt;
    }

    std::size_t latest_index = (head_ == 0) ? (capacity_ - 1) : (head_ - 1);
    return buffer_[latest_index];
}

std::size_t RingBuffer::size() const {
    return count_;
}

std::size_t RingBuffer::capacity() const {
    return capacity_;
}

bool RingBuffer::empty() const {
    return count_ == 0;
}