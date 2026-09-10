#include "http_server.h"
#include "dashboard_html.h"
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
    res.set_content("{\"status\":\"ok\"}", "application/json");
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
    // Return complete JSON with latest, average, and points array for Chart.js
    auto json = std::format(
        "{{\"name\":\"{}\",\"latest\":{},\"average\":{:.2f},\"points\":{}}}",
        name, latest->value, *avg, points_json
    );
    res.set_content(json, "application/json");
}

void HttpServer::handle_index(const httplib::Request&, httplib::Response& res) {
    // Serve embedded Chart.js single-page dashboard directly from memory (zero allocations)
    res.set_content(DASHBOARD_HTML.data(), DASHBOARD_HTML.size(), "text/html");
}