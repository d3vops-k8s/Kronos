#include "http_server.h"
#include "dashboard_html.h"
#include "gorilla.h"
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <charconv>

HttpServer::HttpServer(ThreadSafeStorage& storage, std::uint16_t port)
    : storage_(storage), port_(port) {
    register_routes();
}

void HttpServer::start() {
    std::cout << std::format("[HttpServer] listening on http://localhost:{}\n", port_);
    server_.listen("localhost", port_);
}

void HttpServer::stop() {
    server_.stop();
}

void HttpServer::register_routes() {
    server_.Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        handle_index(req, res);
    });

    server_.Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handle_health(req, res);
    });

    server_.Get("/metrics", [this](const httplib::Request& req, httplib::Response& res) {
        handle_self_metrics(req, res);
    });

    server_.Get("/api/v1/metrics_list", [this](const httplib::Request& req, httplib::Response& res) {
        handle_metrics_list(req, res);
    });

    server_.Get("/api/v1/query", [this](const httplib::Request& req, httplib::Response& res) {
        handle_query(req, res);
    });
}

void HttpServer::handle_health(const httplib::Request&, httplib::Response& res) {
    auto uptime_sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time_
    ).count();
    res.set_content(
        std::format("{{\"status\":\"ok\",\"uptime_seconds\":{}}}", uptime_sec),
        "application/json"
    );
}

void HttpServer::handle_self_metrics(const httplib::Request&, httplib::Response& res) {
    auto uptime_sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time_
    ).count();

    std::size_t series_count = storage_.metric_count();
    std::uint64_t samples_total = storage_.total_samples_ingested();

    std::uintmax_t wal_bytes = 0;
    if (std::filesystem::exists("data/wal/kronos.wal")) {
        wal_bytes = std::filesystem::file_size("data/wal/kronos.wal");
    }

    double compression_ratio = 1.0;
    auto names = storage_.metric_names();
    if (!names.empty()) {
        auto pts = storage_.get_points(names[0]);
        if (!pts.empty()) {
            GorillaCompressor comp;
            for (const auto& p : pts) {
                comp.compress(p);
            }
            auto compressed = comp.finish();
            if (!compressed.empty()) {
                compression_ratio = static_cast<double>(pts.size() * sizeof(MetricPoint)) / static_cast<double>(compressed.size());
            }
        }
    }

    std::string body;
    body += "# HELP kronos_uptime_seconds Total runtime of Kronos TSDB in seconds.\n";
    body += "# TYPE kronos_uptime_seconds gauge\n";
    body += std::format("kronos_uptime_seconds {}\n\n", uptime_sec);

    body += "# HELP kronos_storage_series_count Number of unique time-series metrics currently tracked in RAM.\n";
    body += "# TYPE kronos_storage_series_count gauge\n";
    body += std::format("kronos_storage_series_count {}\n\n", series_count);

    body += "# HELP kronos_samples_ingested_total Total count of data points ingested into storage.\n";
    body += "# TYPE kronos_samples_ingested_total counter\n";
    body += std::format("kronos_samples_ingested_total {}\n\n", samples_total);

    body += "# HELP kronos_wal_file_bytes Size of the Write-Ahead Log on disk in bytes.\n";
    body += "# TYPE kronos_wal_file_bytes gauge\n";
    body += std::format("kronos_wal_file_bytes {}\n\n", wal_bytes);

    body += "# HELP kronos_gorilla_compression_ratio Active compression ratio achieved by Gorilla engine.\n";
    body += "# TYPE kronos_gorilla_compression_ratio gauge\n";
    body += std::format("kronos_gorilla_compression_ratio {:.2f}\n", compression_ratio);

    res.set_content(body, "text/plain; version=0.0.4; charset=utf-8");
}

void HttpServer::handle_metrics_list(const httplib::Request&, httplib::Response& res) {
    auto names = storage_.metric_names();

    std::string json = "[";
    bool first = true;
    for (const auto& name : names) {
        if (name.empty() || name.size() > 256) {
            continue;
        }
        bool valid = true;
        for (char c : name) {
            if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126 || c == '"' || c == '\\') {
                valid = false;
                break;
            }
        }
        if (!valid) {
            continue;
        }
        if (!first) {
            json += ",";
        }
        first = false;
        json += std::format("\"{}\"", name);
    }
    json += "]";

    res.set_content(json, "application/json");
}

void HttpServer::handle_query(const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("name")) {
        res.status = 400;
        res.set_content("{\"error\":\"missing parameter: name\"}", "application/json");
        return;
    }
    auto name   = req.get_param_value("name");
    auto latest = storage_.get_latest(name);
    auto avg    = storage_.get_average(name);

    if (!latest || !avg) {
        res.status = 404;
        res.set_content(
            std::format("{{\"error\":\"metric not found: {}\"}}", name),
            "application/json"
        );
        return;
    }

    // Parse optional query parameters for range filtering: start, end, limit
    std::int64_t start_time = 0;
    std::int64_t end_time   = 0;
    std::size_t  limit      = 0;

    if (req.has_param("start")) {
        auto val = req.get_param_value("start");
        std::from_chars(val.data(), val.data() + val.size(), start_time);
    }
    if (req.has_param("end")) {
        auto val = req.get_param_value("end");
        std::from_chars(val.data(), val.data() + val.size(), end_time);
    }
    if (req.has_param("limit")) {
        auto val = req.get_param_value("limit");
        std::from_chars(val.data(), val.data() + val.size(), limit);
    }

    auto points = storage_.get_points_range(name, start_time, end_time, limit);

    // Build the points array: [{"timestamp": 1717200000, "value": 1245.67}, ...]
    std::string points_json = "[";
    for (std::size_t i = 0; i < points.size(); ++i) {
        points_json += std::format(
            "{{\"timestamp\":{},\"value\":{}}}",
            points[i].timestamp, points[i].value
        );
        if (i + 1 < points.size()) {
            points_json += ",";
        }
    }
    points_json += "]";

    // Live Gorilla compression on active buffer points
    std::size_t raw_bytes = points.size() * sizeof(MetricPoint);
    std::size_t compressed_bytes = raw_bytes;
    double compression_ratio = 1.0;
    double bits_per_point = 128.0;

    if (!points.empty()) {
        GorillaCompressor compressor;
        for (const auto& pt : points) {
            compressor.compress(pt);
        }
        auto compressed = compressor.finish();
        compressed_bytes = compressed.size();
        if (compressed_bytes > 0) {
            compression_ratio = static_cast<double>(raw_bytes) / static_cast<double>(compressed_bytes);
            bits_per_point = (static_cast<double>(compressed_bytes) * 8.0) / static_cast<double>(points.size());
        }
    }

    // Read current WAL file size from disk
    std::uintmax_t wal_bytes = 0;
    if (std::filesystem::exists("data/wal/kronos.wal")) {
        wal_bytes = std::filesystem::file_size("data/wal/kronos.wal");
    }

    // Return complete JSON with latest, average, points, and live engine stats
    auto json = std::format(
        "{{\"name\":\"{}\",\"latest\":{},\"average\":{:.2f},\"points\":{},"
        "\"gorilla\":{{\"raw_bytes\":{},\"compressed_bytes\":{},\"compression_ratio\":{:.2f},\"bits_per_point\":{:.2f}}},"
        "\"wal\":{{\"file_bytes\":{},\"status\":\"synced\"}}}}",
        name, latest->value, *avg, points_json,
        raw_bytes, compressed_bytes, compression_ratio, bits_per_point,
        wal_bytes
    );
    res.set_content(json, "application/json");
}

void HttpServer::handle_index(const httplib::Request&, httplib::Response& res) {
    res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");
    res.set_content(DASHBOARD_HTML.data(), DASHBOARD_HTML.size(), "text/html; charset=utf-8");
}