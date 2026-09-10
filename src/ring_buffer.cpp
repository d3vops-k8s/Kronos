#include "ring_buffer.h"
#include <algorithm>

RingBuffer::RingBuffer(std::size_t capacity)
    : capacity_(capacity == 0 ? 1 : capacity) {
    buffer_.resize(capacity_);
}

void RingBuffer::push(const Sample& sample) {
    buffer_[head_] = sample;
    head_ = (head_ + 1) % capacity_;

    if (count_ < capacity_) {
        ++count_;
    }
}

void RingBuffer::push(std::int64_t timestamp, double value) {
    push(Sample{.timestamp = timestamp, .value = value});
}

std::optional<Sample> RingBuffer::get_latest() const {
    if (empty()) {
        return std::nullopt;
    }
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
    std::size_t start = (count_ < capacity_) ? 0 : head_;

    for (std::size_t i = 0; i < count_; ++i) {
        sum += buffer_[(start + i) % capacity_].value;
    }

    return sum / static_cast<double>(count_);
}

std::vector<Sample> RingBuffer::get_all() const {
    if (empty()) {
        return {};
    }
    std::vector<Sample> samples;
    samples.reserve(count_);
    std::size_t start = (count_ < capacity_) ? 0 : head_;
    for (std::size_t i = 0; i < count_; ++i) {
        samples.push_back(buffer_[(start + i) % capacity_]);
    }
    return samples;
}

std::vector<Sample> RingBuffer::get_range(std::int64_t start_time, std::int64_t end_time, std::size_t limit) const {
    if (empty()) {
        return {};
    }
    std::vector<Sample> samples;
    if (limit > 0) {
        samples.reserve(std::min(limit, count_));
    } else {
        samples.reserve(count_);
    }

    std::size_t start_idx = (count_ < capacity_) ? 0 : head_;
    for (std::size_t i = 0; i < count_; ++i) {
        const auto& s = buffer_[(start_idx + i) % capacity_];
        if (start_time > 0 && s.timestamp < start_time) {
            continue;
        }
        if (end_time > 0 && s.timestamp > end_time) {
            continue;
        }
        samples.push_back(s);
        if (limit > 0 && samples.size() >= limit) {
            break;
        }
    }
    return samples;
}