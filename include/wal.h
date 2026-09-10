#pragma once

#include <cstdint>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <optional>
#include <span>
#include <array>
#include "metric.h"

// Forward declaration
class ThreadSafeStorage;

namespace wal {

inline constexpr std::uint16_t MAGIC   = 0x524B;  // 'K' (0x4B) and 'R' (0x52)
inline constexpr std::uint16_t VERSION = 2;       // Version 2 adds 32-bit CRC checksum per record

// ─── CRC32 Implementation (IEEE 802.3 polynomial 0xEDB88320) ──────────────────
constexpr auto generate_crc32_table() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint32_t c = i;
        for (int j = 0; j < 8; ++j) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        table[i] = c;
    }
    return table;
}

inline constexpr auto CRC32_TABLE = generate_crc32_table();

inline std::uint32_t crc32_update(std::uint32_t crc, const void* data, std::size_t len) {
    auto ptr = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < len; ++i) {
        crc = CRC32_TABLE[(crc ^ ptr[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

inline std::uint32_t calculate_crc32(const void* data, std::size_t len) {
    return ~crc32_update(~0u, data, len);
}

// Low-level binary serialization primitives
bool write_header(std::ostream& out);
bool validate_header(std::istream& in, std::uint16_t& out_version);
bool write_record(std::ostream& out, const MetricPoint& point);
std::optional<MetricPoint> read_record(std::istream& in, std::uint16_t version);

// High-level WAL Writer: thread-safe append-only binary logger with CRC32 integrity.
class WALWriter {
public:
    explicit WALWriter(const std::filesystem::path& path);
    ~WALWriter();

    // Appends a metric point to the WAL file with CRC32. Thread-safe.
    bool append(const MetricPoint& point);

    // Appends a batch of metric points to the WAL with a single lock and flush. Thread-safe.
    bool append_batch(std::span<const MetricPoint> points);

    // Flushes buffered bytes to the underlying OS page cache.
    void flush();

private:
    std::filesystem::path path_;
    std::ofstream         file_;
    std::mutex            mutex_;
};

// High-level WAL Reader: replays saved WAL segments into storage with CRC32 verification.
class WALReader {
public:
    explicit WALReader(const std::filesystem::path& path);

    // Reads all valid records, verifies CRC32 checksums, and inserts into storage.
    // Returns the number of recovered data points.
    std::size_t recover(ThreadSafeStorage& storage);

private:
    std::filesystem::path path_;
};

} // namespace wal