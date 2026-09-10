#include "wal.h"
#include "thread_safe_storage.h"
#include <string>
#include <iostream>
#include <format>
#include <vector>
#include <cmath>

namespace wal {

bool write_header(std::ostream& out) {
    out.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
    out.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));
    return out.good();
}

bool validate_header(std::istream& in, std::uint16_t& out_version) {
    std::uint16_t magic   = 0;
    std::uint16_t version = 0;

    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (!in.good()) {
        return false;
    }

    if (magic != MAGIC) {
        return false;
    }

    // Support backwards compatibility: v1 (no CRC) and v2 (CRC32)
    if (version != 1 && version != 2) {
        return false;
    }

    out_version = version;
    return true;
}

bool write_record(std::ostream& out, const MetricPoint& point) {
    // 1. Write timestamp (int64_t, 8 bytes)
    out.write(reinterpret_cast<const char*>(&point.timestamp), sizeof(point.timestamp));

    // 2. Write value (double, 8 bytes)
    out.write(reinterpret_cast<const char*>(&point.value), sizeof(point.value));

    // 3. Write metric name length (uint16_t, 2 bytes)
    auto name_len = static_cast<std::uint16_t>(point.name.size());
    out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));

    // 4. Write raw metric name characters
    if (name_len > 0) {
        out.write(point.name.data(), name_len);
    }

    // 5. Compute CRC32 over payload
    std::uint32_t crc = ~0u;
    crc = crc32_update(crc, &point.timestamp, sizeof(point.timestamp));
    crc = crc32_update(crc, &point.value, sizeof(point.value));
    crc = crc32_update(crc, &name_len, sizeof(name_len));
    if (name_len > 0) {
        crc = crc32_update(crc, point.name.data(), name_len);
    }
    crc = ~crc;

    // 6. Write CRC32 (uint32_t, 4 bytes)
    out.write(reinterpret_cast<const char*>(&crc), sizeof(crc));

    return out.good();
}

std::optional<MetricPoint> read_record(std::istream& in, std::uint16_t version) {
    MetricPoint point;

    // 1. Read timestamp (8 bytes)
    in.read(reinterpret_cast<char*>(&point.timestamp), sizeof(point.timestamp));
    if (in.gcount() == 0) {
        return std::nullopt; // Clean End-Of-File
    }
    if (in.gcount() < static_cast<std::streamsize>(sizeof(point.timestamp))) {
        std::cerr << "[WAL] Warning: Incomplete timestamp encountered, truncated log.\n";
        return std::nullopt;
    }

    // 2. Read value (8 bytes)
    in.read(reinterpret_cast<char*>(&point.value), sizeof(point.value));
    if (!in.good()) {
        return std::nullopt;
    }

    // Sanity check: value must be finite
    if (!std::isfinite(point.value)) {
        std::cerr << "[WAL] Warning: Corrupted non-finite value encountered in record. Discarding.\n";
        return std::nullopt;
    }

    // 3. Read metric name length (2 bytes)
    std::uint16_t name_len = 0;
    in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
    if (!in.good()) {
        return std::nullopt;
    }

    // Sanity check: metric name must be reasonable (1 to 256 bytes)
    if (name_len == 0 || name_len > 256) {
        std::cerr << std::format("[WAL] Warning: Corrupted metric name length: {}. Discarding.\n", name_len);
        return std::nullopt;
    }

    // 4. Read metric name characters
    point.name.resize(name_len);
    in.read(point.name.data(), name_len);
    if (!in.good()) {
        return std::nullopt;
    }

    // Sanity check: metric name must contain only valid printable characters
    for (char c : point.name) {
        if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
            std::cerr << "[WAL] Warning: Corrupted metric name with non-printable characters. Discarding.\n";
            return std::nullopt;
        }
    }

    // 5. For Version 2+, verify CRC32
    if (version >= 2) {
        std::uint32_t expected_crc = 0;
        in.read(reinterpret_cast<char*>(&expected_crc), sizeof(expected_crc));
        if (!in.good()) {
            std::cerr << "[WAL] Warning: Incomplete record: missing CRC32 checksum.\n";
            return std::nullopt;
        }

        std::uint32_t actual_crc = ~0u;
        actual_crc = crc32_update(actual_crc, &point.timestamp, sizeof(point.timestamp));
        actual_crc = crc32_update(actual_crc, &point.value, sizeof(point.value));
        actual_crc = crc32_update(actual_crc, &name_len, sizeof(name_len));
        if (name_len > 0) {
            actual_crc = crc32_update(actual_crc, point.name.data(), name_len);
        }
        actual_crc = ~actual_crc;

        if (actual_crc != expected_crc) {
            std::cerr << std::format("[WAL] CRC32 MISMATCH in record for '{}': expected {:#x}, got {:#x}. Corrupted record discarded.\n",
                point.name, expected_crc, actual_crc);
            return std::nullopt;
        }
    }

    return point;
}

// ─── WALWriter ────────────────────────────────────────────────────────────────

WALWriter::WALWriter(const std::filesystem::path& path)
    : path_(path) {
    if (path_.has_parent_path()) {
        std::filesystem::create_directories(path_.parent_path());
    }

    bool file_exists = std::filesystem::exists(path_);
    bool is_empty    = !file_exists || (std::filesystem::file_size(path_) == 0);
    bool need_header = is_empty;

    if (file_exists && !is_empty) {
        std::ifstream test(path_, std::ios::binary);
        std::uint16_t existing_version = 0;
        if (!validate_header(test, existing_version) || existing_version != VERSION) {
            test.close();
            // Existing WAL has a mismatched version or corrupt header.
            // Safely archive the old file and start a fresh VERSION log.
            std::error_code ec;
            auto archive_path = path_;
            archive_path += ".v1.bak";
            std::filesystem::rename(path_, archive_path, ec);
            std::cout << "[WAL] Archived older version WAL to: " << archive_path.string() << "\n";
            need_header = true;
        }
    }

    if (need_header) {
        file_.open(path_, std::ios::binary | std::ios::out | std::ios::trunc);
        if (file_.is_open()) {
            write_header(file_);
            file_.flush();
        }
    } else {
        file_.open(path_, std::ios::binary | std::ios::app);
    }
}

WALWriter::~WALWriter() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

bool WALWriter::append(const MetricPoint& point) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) {
        return false;
    }

    if (!write_record(file_, point)) {
        return false;
    }

    file_.flush(); // Ensure persistence to OS buffers
    return true;
}

bool WALWriter::append_batch(std::span<const MetricPoint> points) {
    if (points.empty()) {
        return true;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) {
        return false;
    }

    for (const auto& point : points) {
        if (!write_record(file_, point)) {
            return false;
        }
    }

    file_.flush(); // Ensure persistence to OS buffers once per batch
    return true;
}

void WALWriter::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

// ─── WALReader ────────────────────────────────────────────────────────────────

WALReader::WALReader(const std::filesystem::path& path)
    : path_(path) {}

std::size_t WALReader::recover(ThreadSafeStorage& storage) {
    if (!std::filesystem::exists(path_)) {
        return 0;
    }

    std::ifstream file(path_, std::ios::binary);
    if (!file.is_open()) {
        return 0;
    }

    std::uint16_t version = 0;
    if (!validate_header(file, version)) {
        std::cerr << "[WALReader] Warning: Invalid or missing WAL header in " << path_ << "\n";
        return 0;
    }

    std::size_t count = 0;
    std::vector<MetricPoint> batch;
    batch.reserve(256);

    while (auto point = read_record(file, version)) {
        batch.push_back(std::move(*point));
        if (batch.size() >= 256) {
            storage.insert_batch(batch);
            count += batch.size();
            batch.clear();
        }
    }

    if (!batch.empty()) {
        count += batch.size();
        storage.insert_batch(batch);
    }

    return count;
}

} // namespace wal