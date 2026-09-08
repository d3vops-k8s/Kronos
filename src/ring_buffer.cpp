#include "ring_buffer.h"

RingBuffer::RingBuffer(std::size_t capacity)
    // Guard against capacity == 0 to avoid modulo-by-zero in push().
    : capacity_(capacity == 0 ? 1 : capacity) {
    buffer_.resize(capacity_);  // Single heap allocation for the lifetime of the buffer.
}

void RingBuffer::push(const MetricPoint& point) {
    buffer_[head_] = point;
    head_ = (head_ + 1) % capacity_;  // Wrap-around using modular arithmetic.

    // count_ saturates at capacity_; once full, it stays there.
    if (count_ < capacity_) {
        ++count_;
    }
}

std::optional<MetricPoint> RingBuffer::get_latest() const {
    if (empty()) {
        return std::nullopt;
    }
    // head_ points to the next write slot, so the last written element is at head_ - 1.
    std::size_t latest_index = (head_ == 0) ? (capacity_ - 1) : (head_ - 1);
    return buffer_[latest_index];
}

std::size_t RingBuffer::size()     const { return count_;    }
std::size_t RingBuffer::capacity() const { return capacity_; }
bool        RingBuffer::empty()    const { return count_ == 0; }

std::optional<double> RingBuffer::average() const {
    if (empty()) {
        return std::nullopt;
    }

    double sum = 0.0;

    // Determine the start index for chronological traversal.
    // If the buffer is not yet full, valid data starts at index 0.
    // If the buffer is full, the oldest entry is at head_ (the next overwrite position).
    std::size_t start = (count_ < capacity_) ? 0 : head_;

    for (std::size_t i = 0; i < count_; ++i) {
        sum += buffer_[(start + i) % capacity_].value;
    }

    return sum / static_cast<double>(count_);
}

std::vector<MetricPoint> RingBuffer::get_all() const {
    if (empty()) {
        return {};
    }
    std::vector<MetricPoint> points;
    points.reserve(count_);  // Single allocation for the exact number of elements.
    // If the buffer is full, the oldest element is at head_; otherwise it is at 0.
    std::size_t start = (count_ < capacity_) ? 0 : head_;
    for (std::size_t i = 0; i < count_; ++i) {
        points.push_back(buffer_[(start + i) % capacity_]);
    }
    return points;
}