#include <iostream>
#include <format>
#include <csignal>
#include <filesystem>
#include "thread_safe_storage.h"
#include "scraper.h"
#include "http_server.h"
#include "wal.h"

using namespace std::chrono_literals;

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

    std::cout << "=== Kronos TSDB Engine with Write-Ahead Log (C++23) ===\n\n";

    ThreadSafeStorage storage;
    const std::filesystem::path wal_path = "data/wal/kronos.wal";

    // ─── 1. Recovery Phase (Crash Resilience) ──────────────────────────────────
    std::cout << "[WAL] Checking for existing WAL log at: " << wal_path << " ...\n";
    wal::WALReader reader(wal_path);
    std::size_t recovered = reader.recover(storage);

    if (recovered > 0) {
        std::cout << std::format("[WAL] ✅ RECOVERY COMPLETE: Restored {} points from disk!\n", recovered);
        std::cout << std::format("[WAL] Active metrics in storage: {}\n\n", storage.metric_count());
    } else {
        std::cout << "[WAL] ℹ️ Clean start (no previous log found).\n\n";
    }

    // ─── 2. Attach WAL Writer for live incoming metrics ───────────────────────
    wal::WALWriter writer(wal_path);
    storage.set_wal(&writer);

    // ─── 3. Start Scraper (polls mock_exporter on port 9100) ──────────────────
    ScrapeConfig scrape_config{
        .host     = "localhost",
        .port     = 9100,
        .path     = "/metrics",
        .interval = 2s
    };

    Scraper scraper(storage, scrape_config);
    scraper.start();

    // ─── 4. Start HTTP Server (port 8080) ────────────────────────────────────
    HttpServer server(storage, 8080);
    g_server = &server;

    std::cout << "[Kronos] Scraper running -> target: http://localhost:9100/metrics\n";
    std::cout << "[Kronos] Web UI live at http://localhost:8080\n";
    std::cout << "[Kronos] Press Ctrl+C to test graceful shutdown.\n\n";

    server.start();

    // ─── 5. Cleanup ──────────────────────────────────────────────────────────
    scraper.stop();
    storage.set_wal(nullptr);
    std::cout << "[Kronos] Daemon stopped cleanly.\n";

    return 0;
}