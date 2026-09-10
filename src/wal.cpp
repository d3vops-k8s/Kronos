#include "wal.h"
#include "thread_safe_storage.h"
#include <string>
#include <iostream>

namespace wal {

bool write_header(std::ostream& out) {
    out.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
    out.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));
    return out.good();
}

bool validate_header(std::istream& in) {
    std::uint16_t magic   = 0;
    std::uint16_t version = 0;

    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (!in.good()) {
        return false;
    }

    return (magic == MAGIC) && (version == VERSION);
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

    return out.good();
}

std::optional<MetricPoint> read_record(std::istream& in) {
    MetricPoint point;

    // 1. Read timestamp (8 bytes)
    in.read(reinterpret_cast<char*>(&point.timestamp), sizeof(point.timestamp));
    if (in.gcount() == 0) {
        return std::nullopt; // Clean End-Of-File
    }
    if (in.gcount() < static_cast<std::streamsize>(sizeof(point.timestamp))) {
        return std::nullopt; // Corrupted / incomplete record
    }

    // 2. Read value (8 bytes)
    in.read(reinterpret_cast<char*>(&point.value), sizeof(point.value));
    if (!in.good()) {
        return std::nullopt;
    }

    // 3. Read metric name length (2 bytes)
    std::uint16_t name_len = 0;
    in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
    if (!in.good()) {
        return std::nullopt;
    }

    // 4. Read metric name characters
    point.name.resize(name_len);
    if (name_len > 0) {
        in.read(point.name.data(), name_len);
        if (!in.good()) {
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

    file_.open(path_, std::ios::binary | std::ios::app);
    if (file_.is_open() && is_empty) {
        write_header(file_);
        file_.flush();
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

    if (!validate_header(file)) {
        std::cerr << "[WALReader] Warning: Invalid or missing WAL header in " << path_ << "\n";
        return 0;
    }

    std::size_t count = 0;
    while (auto point = read_record(file)) {
        storage.insert(*point);
        ++count;
    }

    return count;
}

} // namespace wal