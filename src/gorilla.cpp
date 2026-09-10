#include "gorilla.h"
#include <bit>
#include <cstring>

// ─── GorillaCompressor ────────────────────────────────────────────────────────

void GorillaCompressor::compress(const MetricPoint& point) {
    compress_timestamp(point.timestamp);
    compress_value(point.value);
    ++count_;
}

void GorillaCompressor::compress_timestamp(std::int64_t timestamp) {
    if (count_ == 0) {
        // First timestamp: write full 64 bits
        writer_.write_bits(static_cast<std::uint64_t>(timestamp), 64);
        prev_timestamp_ = timestamp;
        return;
    }

    std::int64_t cur_delta = timestamp - prev_timestamp_;

    if (count_ == 1) {
        // First delta: write full 32 bits
        writer_.write_bits(static_cast<std::uint64_t>(cur_delta), 32);
        prev_delta_     = cur_delta;
        prev_timestamp_ = timestamp;
        return;
    }

    // Delta-of-Delta: D' = (t_i - t_{i-1}) - (t_{i-1} - t_{i-2})
    std::int64_t dod = cur_delta - prev_delta_;

    if (dod == 0) {
        // Case 1: Constant scrape interval (e.g. exactly 2s) -> 1 bit '0'
        writer_.write_bit(false);
    } else if (dod >= -63 && dod <= 64) {
        // Case 2: dod fits in 7 bits -> '10' + 7 bits
        writer_.write_bits(0b10, 2);
        writer_.write_bits(static_cast<std::uint64_t>(dod + 63), 7);
    } else if (dod >= -255 && dod <= 256) {
        // Case 3: dod fits in 9 bits -> '110' + 9 bits
        writer_.write_bits(0b110, 3);
        writer_.write_bits(static_cast<std::uint64_t>(dod + 255), 9);
    } else if (dod >= -2047 && dod <= 2048) {
        // Case 4: dod fits in 12 bits -> '1110' + 12 bits
        writer_.write_bits(0b1110, 4);
        writer_.write_bits(static_cast<std::uint64_t>(dod + 2047), 12);
    } else {
        // Case 5: large time jump -> '1111' + 32 bits
        writer_.write_bits(0b1111, 4);
        writer_.write_bits(static_cast<std::uint64_t>(dod), 32);
    }

    prev_delta_     = cur_delta;
    prev_timestamp_ = timestamp;
}

void GorillaCompressor::compress_value(double value) {
    std::uint64_t cur_val = 0;
    std::memcpy(&cur_val, &value, sizeof(double));

    if (count_ == 0) {
        // First value: write full 64 bits
        writer_.write_bits(cur_val, 64);
        prev_value_ = cur_val;
        return;
    }

    std::uint64_t xor_val = cur_val ^ prev_value_;

    if (xor_val == 0) {
        // Case 1: Value is identical to previous -> 1 bit '0'
        writer_.write_bit(false);
    } else {
        // Case 2: Value changed -> 1 bit '1' followed by significant bits
        writer_.write_bit(true);

        int lz  = std::countl_zero(xor_val); // Leading zeros (0..63)
        int tz  = std::countr_zero(xor_val); // Trailing zeros (0..63)
        int len = 64 - lz - tz;             // Significant bits length (1..64)

        writer_.write_bits(static_cast<std::uint64_t>(lz), 6);
        writer_.write_bits(static_cast<std::uint64_t>(len == 64 ? 0 : len), 6);

        std::uint64_t meaningful = (xor_val >> tz);
        writer_.write_bits(meaningful, static_cast<std::uint8_t>(len));

        prev_value_ = cur_val;
    }
}

std::vector<std::uint8_t> GorillaCompressor::finish() {
    writer_.flush();
    return writer_.bytes();
}

std::size_t GorillaCompressor::count() const {
    return count_;
}

// ─── GorillaDecompressor ──────────────────────────────────────────────────────

std::vector<MetricPoint> GorillaDecompressor::decompress(
    std::span<const std::uint8_t> bytes,
    const std::string& metric_name,
    std::size_t expected_count
) {
    if (expected_count == 0 || bytes.empty()) {
        return {};
    }

    BitReader reader(bytes);
    std::vector<MetricPoint> points;
    points.reserve(expected_count);

    std::int64_t  prev_timestamp = 0;
    std::int64_t  prev_delta     = 0;
    std::uint64_t prev_value     = 0;

    for (std::size_t i = 0; i < expected_count; ++i) {
        MetricPoint pt;
        pt.name = metric_name;

        // 1. Decompress timestamp
        if (i == 0) {
            auto ts_bits = reader.read_bits(64);
            if (!ts_bits) break;
            pt.timestamp   = static_cast<std::int64_t>(*ts_bits);
            prev_timestamp = pt.timestamp;
        } else if (i == 1) {
            auto delta_bits = reader.read_bits(32);
            if (!delta_bits) break;
            prev_delta     = static_cast<std::int32_t>(*delta_bits);
            pt.timestamp   = prev_timestamp + prev_delta;
            prev_timestamp = pt.timestamp;
        } else {
            auto b0 = reader.read_bit();
            if (!b0) break;

            std::int64_t dod = 0;
            if (!*b0) {
                dod = 0; // '0' -> dod = 0
            } else {
                auto b1 = reader.read_bit();
                if (!b1) break;
                if (!*b1) {
                    // '10' -> 7 bits
                    auto bits = reader.read_bits(7);
                    if (!bits) break;
                    dod = static_cast<std::int64_t>(*bits) - 63;
                } else {
                    auto b2 = reader.read_bit();
                    if (!b2) break;
                    if (!*b2) {
                        // '110' -> 9 bits
                        auto bits = reader.read_bits(9);
                        if (!bits) break;
                        dod = static_cast<std::int64_t>(*bits) - 255;
                    } else {
                        auto b3 = reader.read_bit();
                        if (!b3) break;
                        if (!*b3) {
                            // '1110' -> 12 bits
                            auto bits = reader.read_bits(12);
                            if (!bits) break;
                            dod = static_cast<std::int64_t>(*bits) - 2047;
                        } else {
                            // '1111' -> 32 bits
                            auto bits = reader.read_bits(32);
                            if (!bits) break;
                            dod = static_cast<std::int32_t>(*bits);
                        }
                    }
                }
            }

            prev_delta    += dod;
            pt.timestamp   = prev_timestamp + prev_delta;
            prev_timestamp = pt.timestamp;
        }

        // 2. Decompress value
        if (i == 0) {
            auto val_bits = reader.read_bits(64);
            if (!val_bits) break;
            prev_value = *val_bits;
            std::memcpy(&pt.value, &prev_value, sizeof(double));
        } else {
            auto changed = reader.read_bit();
            if (!changed) break;

            if (!*changed) {
                // Value unchanged
                std::memcpy(&pt.value, &prev_value, sizeof(double));
            } else {
                auto lz_bits  = reader.read_bits(6);
                auto len_bits = reader.read_bits(6);
                if (!lz_bits || !len_bits) break;

                int lz  = static_cast<int>(*lz_bits);
                int len = static_cast<int>(*len_bits);
                if (len == 0) len = 64;

                auto meaningful_bits = reader.read_bits(static_cast<std::uint8_t>(len));
                if (!meaningful_bits) break;

                int tz = 64 - lz - len;
                std::uint64_t xor_val = (*meaningful_bits) << tz;
                std::uint64_t cur_val = prev_value ^ xor_val;

                std::memcpy(&pt.value, &cur_val, sizeof(double));
                prev_value = cur_val;
            }
        }

        points.push_back(std::move(pt));
    }

    return points;
}