#include <iostream>
#include <format>
#include <vector>
#include <chrono>
#include <cmath>
#include "gorilla.h"
#include "metric.h"

int main() {
    std::cout << "=== Kronos TSDB — Gorilla Compression Benchmark ===\n\n";

    constexpr std::size_t TOTAL_POINTS = 10'000;
    std::vector<MetricPoint> raw_points;
    raw_points.reserve(TOTAL_POINTS);

    // Simulate 10,000 realistic points (every 2s, CPU fluctuating smoothly around 45.0%)
    std::int64_t base_time = 1717200000;
    double current_value = 45.25;

    for (std::size_t i = 0; i < TOTAL_POINTS; ++i) {
        // Occasional slight drift in value
        if (i % 5 == 0) {
            current_value += ((i % 3 == 0) ? 0.15 : -0.10);
        }

        MetricPoint pt{
            .name      = "node_cpu_seconds_total",
            .value     = current_value,
            .timestamp = base_time + static_cast<std::int64_t>(i * 2) // Steady 2s scrape interval
        };
        raw_points.push_back(pt);
    }

    // ─── 1. Compression ──────────────────────────────────────────────────────────
    auto t0 = std::chrono::high_resolution_clock::now();

    GorillaCompressor compressor;
    for (const auto& pt : raw_points) {
        compressor.compress(pt);
    }
    auto compressed_bytes = compressor.finish();

    auto t1 = std::chrono::high_resolution_clock::now();
    auto comp_time_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    // ─── 2. Decompression ────────────────────────────────────────────────────────
    auto t2 = std::chrono::high_resolution_clock::now();

    auto decompressed_points = GorillaDecompressor::decompress(
        compressed_bytes,
        "node_cpu_seconds_total",
        compressor.count()
    );

    auto t3 = std::chrono::high_resolution_clock::now();
    auto decomp_time_us = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();

    // ─── 3. Verification ─────────────────────────────────────────────────────────
    bool exact_match = (raw_points.size() == decompressed_points.size());
    if (exact_match) {
        for (std::size_t i = 0; i < raw_points.size(); ++i) {
            if (raw_points[i].timestamp != decompressed_points[i].timestamp ||
                raw_points[i].value != decompressed_points[i].value) {
                exact_match = false;
                break;
            }
        }
    }

    // ─── 4. Results ──────────────────────────────────────────────────────────────
    std::size_t raw_size = TOTAL_POINTS * (sizeof(double) + sizeof(std::int64_t)); // 16 bytes per point
    std::size_t compressed_size = compressed_bytes.size();
    double ratio = static_cast<double>(raw_size) / static_cast<double>(compressed_size);
    double bits_per_point = (static_cast<double>(compressed_size) * 8.0) / static_cast<double>(TOTAL_POINTS);

    std::cout << std::format("Total points processed : {}\n", TOTAL_POINTS);
    std::cout << std::format("Raw data size          : {} bytes ({:.2f} KB)\n", raw_size, raw_size / 1024.0);
    std::cout << std::format("Gorilla compressed size: {} bytes ({:.2f} KB)\n", compressed_size, compressed_size / 1024.0);
    std::cout << std::format("Compression Ratio      : {:.2f}x (saved {:.1f}%)\n", ratio, (1.0 - (1.0 / ratio)) * 100.0);
    std::cout << std::format("Bits per point         : {:.2f} bits (down from 128 bits!)\n\n", bits_per_point);

    std::cout << std::format("Compression time       : {} µs ({:.0f} points/sec)\n",
        comp_time_us, (TOTAL_POINTS * 1e6) / comp_time_us);
    std::cout << std::format("Decompression time     : {} µs ({:.0f} points/sec)\n\n",
        decomp_time_us, (TOTAL_POINTS * 1e6) / decomp_time_us);

    std::cout << std::format("Lossless Accuracy      : {}\n",
        exact_match ? "✅ 100% PERFECT MATCH (0 data loss)" : "❌ MISMATCH DETECTED");

    return exact_match ? 0 : 1;
}