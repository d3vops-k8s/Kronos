// tests/mock_exporter.cpp
//
// Мок-сервер, имитирующий node_exporter Prometheus.
// Запускается в отдельном терминале перед основным kronos.exe.
//
// Аналог: в Kubernetes это был бы отдельный Pod с node_exporter,
// но для локальной разработки мы поднимаем его сами.
//
// Использование:
//   Терминал 1: .\build\mock_exporter.exe
//   Терминал 2: .\build\kronos.exe

#include <httplib.h>
#include <iostream>
#include <format>
#include <random>
#include <chrono>
#include <cmath>

int main() {
    httplib::Server server;

    // Генератор псевдослучайных чисел — имитируем реальные колебания метрик
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> cpu_dist(0.0, 100.0);
    std::uniform_real_distribution<double> mem_dist(4e9, 16e9);
    std::uniform_real_distribution<double> disk_dist(1e6, 1e9);
    std::uniform_real_distribution<double> net_dist(1e5, 1e8);

    // GET /metrics — главный эндпоинт в формате OpenMetrics (как у настоящего node_exporter)
    server.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        // Генерируем реалистичный OpenMetrics ответ с комментариями и случайными значениями
        std::string body;

        body += std::format("# HELP node_cpu_seconds_total Seconds the CPUs spent in each mode\n");
        body += std::format("# TYPE node_cpu_seconds_total counter\n");
        body += std::format("node_cpu_seconds_total {} {}\n", cpu_dist(rng), ts);

        body += std::format("# HELP node_memory_MemTotal_bytes Total installed memory\n");
        body += std::format("# TYPE node_memory_MemTotal_bytes gauge\n");
        body += std::format("node_memory_MemTotal_bytes {} {}\n", mem_dist(rng), ts);

        body += std::format("# HELP node_disk_read_bytes_total Total bytes read from disk\n");
        body += std::format("# TYPE node_disk_read_bytes_total counter\n");
        body += std::format("node_disk_read_bytes_total {} {}\n", disk_dist(rng), ts);

        body += std::format("# HELP node_network_receive_bytes_total Network bytes received\n");
        body += std::format("# TYPE node_network_receive_bytes_total counter\n");
        body += std::format("node_network_receive_bytes_total {} {}\n", net_dist(rng), ts);

        res.set_content(body, "text/plain; version=0.0.4; charset=utf-8");
        std::cout << std::format("[MockExporter] Served /metrics (ts={})\n", ts);
    });

    // GET /health — liveness probe (используется в Kubernetes для проверки живости Pod)
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    std::cout << "[MockExporter] Listening on http://localhost:9100\n";
    std::cout << "[MockExporter] Endpoints: /metrics, /health\n";
    std::cout << "[MockExporter] Press Ctrl+C to stop.\n\n";

    // Блокирующий вызов — сервер работает до Ctrl+C
    server.listen("localhost", 9100);

    return 0;
}
