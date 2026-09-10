#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <string>
#include "bit_stream.h"
#include "metric.h"

// Gorilla time-series compression engine.
// Implements:
//   1. Delta-of-Delta encoding for timestamps (constant scrape intervals compress to 1 bit).
//   2. XOR bit-packing for IEEE 754 double precision values.
//
// Reference: "Gorilla: A Fast, Scalable, In-Memory Time Series Database" (Facebook, 2015).

class GorillaCompressor {
public:
    GorillaCompressor() = default;

    // Compresses a metric point into the internal bit stream.
    void compress(const MetricPoint& point);

    // Finalizes compression, flushes bit buffer, and returns packed bytes.
    std::vector<std::uint8_t> finish();

    // Returns total count of compressed points.
    std::size_t count() const;

private:
    void compress_timestamp(std::int64_t timestamp);
    void compress_value(double value);

    BitWriter    writer_;
    std::size_t  count_           = 0;

    // Timestamp compression state
    std::int64_t prev_timestamp_  = 0;
    std::int64_t prev_delta_      = 0;

    // Value compression state
    std::uint64_t prev_value_     = 0;
};

class GorillaDecompressor {
public:
    // Decompresses a Gorilla-encoded byte buffer back into a vector of MetricPoints.
    static std::vector<MetricPoint> decompress(
        std::span<const std::uint8_t> bytes,
        const std::string& metric_name,
        std::size_t expected_count
    );
};