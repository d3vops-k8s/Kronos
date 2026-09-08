#include <iostream>
#include <format>
#include <chrono>
#include <csignal>
#include "thread_safe_storage.h"
#include "scraper.h"
#include "http_server.h"

using namespace std::chrono_literals;

// Global pointer for clean SIGINT / Ctrl+C handling
HttpServer* g_server = nullptr;

void signal_handler(int) {
    std::cout << "\n[Kronos] Shutdown signal received, stopping server...\n";
    if (g_server) {
        g_server->stop();
    }
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "=== Kronos TSDB Daemon (C++23) ===\n\n";

    ThreadSafeStorage storage;

    // 1. Configure and start background Scraper (polls mock_exporter every 2 seconds)
    ScrapeConfig scrape_config{
        .host     = "localhost",
        .port     = 9100,
        .path     = "/metrics",
        .interval = 2s
    };

    Scraper scraper(storage, scrape_config);
    scraper.start();

    // 2. Configure and start embedded HTTP server on port 8080
    HttpServer server(storage, 8080);
    g_server = &server;

    std::cout << "[Kronos] Scraper running in background -> target: http://localhost:9100/metrics\n";
    std::cout << "[Kronos] Web UI live at http://localhost:8080\n";
    std::cout << "[Kronos] Press Ctrl+C to stop daemon gracefully.\n\n";

    // Blocks main thread while serving HTTP requests
    server.start();

    // Clean shutdown: RAII jthread in scraper stops automatically
    scraper.stop();
    std::cout << "[Kronos] Daemon stopped cleanly.\n";

    return 0;
}