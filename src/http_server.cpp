#include "http_server.h"
#include "dashboard_html.h"
#include "gorilla.h"
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

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

void HttpServer::handle_metrics_list(const httplib::Request&, httplib::Response& res) {
    auto names = storage_.metric_names();

    std::string json = "[";
    for (std::size_t i = 0; i < names.size(); ++i) {
        json += std::format("\"{}\"", names[i]);
        if (i + 1 < names.size()) {
            json += ",";
        }
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
    auto points = storage_.get_points(name);
    if (!latest || !avg) {
        res.status = 404;
        res.set_content(
            std::format("{{\"error\":\"metric not found: {}\"}}", name),
            "application/json"
        );
        return;
    }
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