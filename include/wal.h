#pragma once

#include <cstdint>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <optional>
#include "metric.h"

// Forward declaration
class ThreadSafeStorage;

namespace wal {

inline constexpr std::uint16_t MAGIC   = 0x524B;  // 'K' (0x4B) and 'R' (0x52)
inline constexpr std::uint16_t VERSION = 1;

// Low-level binary serialization primitives
bool write_header(std::ostream& out);
bool validate_header(std::istream& in);
bool write_record(std::ostream& out, const MetricPoint& point);
std::optional<MetricPoint> read_record(std::istream& in);

// High-level WAL Writer: thread-safe append-only binary logger.
class WALWriter {
public:
    explicit WALWriter(const std::filesystem::path& path);
    ~WALWriter();

    // Appends a metric point to the WAL file. Thread-safe.
    bool append(const MetricPoint& point);

    // Flushes buffered bytes to the underlying OS page cache.
    void flush();

private:
    std::filesystem::path path_;
    std::ofstream         file_;
    std::mutex            mutex_;
};

// High-level WAL Reader: replays saved WAL segments into storage.
class WALReader {
public:
    explicit WALReader(const std::filesystem::path& path);

    // Reads all valid records and inserts them into storage.
    // Returns the number of recovered data points.
    std::size_t recover(ThreadSafeStorage& storage);

private:
    std::filesystem::path path_;
};

} // namespace wal