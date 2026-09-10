#include <iostream>
#include <format>
#include <csignal>
#include <filesystem>
#include "config.h"
#include "thread_safe_storage.h"
#include "scraper.h"
#include "http_server.h"
#include "wal.h"

HttpServer* g_server = nullptr;

void signal_handler(int) {
    std::cout << "\n[Kronos] Shutdown signal received, stopping server...\n";
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    // ─── 0. Parse Command-Line Options ─────────────────────────────────────────
    auto config_opt = parse_cli_args(argc, argv);
    if (!config_opt) {
        return 1;
    }
    const auto& config = *config_opt;

    if (config.show_help) {
        print_help(argv[0]);
        return 0;
    }
    if (config.show_version) {
        print_version();
        return 0;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "=== Kronos TSDB Engine with Write-Ahead Log (C++23) ===\n\n";
    std::cout << std::format("[Config] Web UI Port     : {}\n", config.http_port);
    std::cout << std::format("[Config] Scrape Target   : http://{}:{}{}\n", config.scrape_host, config.scrape_port, config.scrape_path);
    std::cout << std::format("[Config] Scrape Interval : {}s\n", config.scrape_interval.count());
    std::cout << std::format("[Config] Series Capacity : {} pts/series\n", config.storage_capacity);
    std::cout << std::format("[Config] WAL File Path   : {}\n\n", config.wal_path.string());

    ThreadSafeStorage storage(config.storage_capacity);

    // ─── 1. Recovery Phase (Crash Resilience) ──────────────────────────────────
    std::cout << "[WAL] Checking for existing WAL log at: " << config.wal_path << " ...\n";
    wal::WALReader reader(config.wal_path);
    std::size_t recovered = reader.recover(storage);

    if (recovered > 0) {
        std::cout << std::format("[WAL] ✅ RECOVERY COMPLETE: Restored {} points from disk!\n", recovered);
        std::cout << std::format("[WAL] Active metrics in storage: {}\n\n", storage.metric_count());
    } else {
        std::cout << "[WAL] ℹ️ Clean start (no previous log found).\n\n";
    }

    // ─── 2. Attach WAL Writer for live incoming metrics ───────────────────────
    wal::WALWriter writer(config.wal_path);
    storage.set_wal(&writer);

    // ─── 3. Start Scraper (polls target on configured interval) ───────────────
    ScrapeConfig scrape_config{
        .host     = config.scrape_host,
        .port     = config.scrape_port,
        .path     = config.scrape_path,
        .interval = config.scrape_interval
    };

    Scraper scraper(storage, scrape_config);
    scraper.start();

    // ─── 4. Start HTTP Server ────────────────────────────────────────────────
    HttpServer server(storage, config.http_port);
    g_server = &server;

    std::cout << std::format("[Kronos] Scraper running -> target: http://{}:{}{}\n",
        config.scrape_host, config.scrape_port, config.scrape_path);
    std::cout << std::format("[Kronos] Web UI live at http://localhost:{}\n", config.http_port);
    std::cout << "[Kronos] Press Ctrl+C to test graceful shutdown.\n\n";

    server.start();

    // ─── 5. Cleanup ──────────────────────────────────────────────────────────
    scraper.stop();
    storage.set_wal(nullptr);
    std::cout << "[Kronos] Daemon stopped cleanly.\n";

    return 0;
}