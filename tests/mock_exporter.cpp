// Mock Prometheus-compatible exporter for integration testing.
//
// Serves randomised metric values in OpenMetrics text format on
// http://localhost:9100/metrics. Intended to be run alongside kronos.exe
// during local development to simulate a real node_exporter target.
//
// Usage:
//   Terminal 1: .\build\mock_exporter.exe
//   Terminal 2: .\build\kronos.exe

#include <httplib.h>
#include <iostream>
#include <format>
#include <random>
#include <chrono>

int main() {
    httplib::Server server;

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> cpu_dist(0.0, 100.0);
    std::uniform_real_distribution<double> mem_dist(4e9, 16e9);
    std::uniform_real_distribution<double> disk_dist(1e6, 1e9);
    std::uniform_real_distribution<double> net_dist(1e5, 1e8);

    // GET /metrics — returns fresh randomised metrics on every request.
    server.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::string body;
        body += "# HELP node_cpu_seconds_total Seconds the CPUs spent in each mode.\n";
        body += "# TYPE node_cpu_seconds_total counter\n";
        body += std::format("node_cpu_seconds_total {} {}\n", cpu_dist(rng), ts);

        body += "# HELP node_memory_MemTotal_bytes Total installed memory.\n";
        body += "# TYPE node_memory_MemTotal_bytes gauge\n";
        body += std::format("node_memory_MemTotal_bytes {} {}\n", mem_dist(rng), ts);

        body += "# HELP node_disk_read_bytes_total Total bytes read from disk.\n";
        body += "# TYPE node_disk_read_bytes_total counter\n";
        body += std::format("node_disk_read_bytes_total {} {}\n", disk_dist(rng), ts);

        body += "# HELP node_network_receive_bytes_total Network bytes received.\n";
        body += "# TYPE node_network_receive_bytes_total counter\n";
        body += std::format("node_network_receive_bytes_total {} {}\n", net_dist(rng), ts);

        res.set_content(body, "text/plain; version=0.0.4; charset=utf-8");
        std::cout << std::format("[MockExporter] served /metrics (ts={})\n", ts);
    });

    // GET /health — liveness probe endpoint (mirrors Kubernetes liveness check convention).
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    std::cout << "[MockExporter] listening on http://localhost:9100\n";
    server.listen("localhost", 9100);
}
