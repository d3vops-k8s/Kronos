#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <charconv>

struct AppConfig {
    std::uint16_t         http_port        = 8080;
    std::string           scrape_host      = "localhost";
    std::uint16_t         scrape_port      = 9100;
    std::string           scrape_path      = "/metrics";
    std::chrono::seconds  scrape_interval  {2};
    std::filesystem::path wal_path         = "data/wal/kronos.wal";
    std::size_t           storage_capacity = 1000;
    bool                  show_help        = false;
    bool                  show_version     = false;
};

inline void print_version() {
    std::cout << "Kronos TSDB Engine v1.0.0 (C++23)\n"
              << "High-performance embedded time-series database with Prometheus scraper,\n"
              << "crash-safe WAL, Gorilla compression, and real-time dashboard.\n";
}

inline void print_help(const char* prog_name) {
    std::cout << "Usage: " << (prog_name ? prog_name : "kronos") << " [OPTIONS]\n\n"
              << "Kronos — High-Performance Embedded Time-Series Engine (C++23)\n\n"
              << "Server Options:\n"
              << "  -p,    --port <port>          HTTP server and Web UI port (default: 8080)\n"
              << "  -c,    --capacity <num>       Ring buffer capacity per metric series (default: 1000)\n"
              << "  -w,    --wal-path <path>      Path to binary Write-Ahead Log (default: data/wal/kronos.wal)\n\n"
              << "Scraper Options:\n"
              << "  -t,    --target-host <host>   Prometheus scrape target host (default: localhost)\n"
              << "  -tp,   --target-port <port>   Prometheus scrape target port (default: 9100)\n"
              << "  -path, --target-path <path>   Prometheus scrape endpoint path (default: /metrics)\n"
              << "  -i,    --interval <sec>       Scrape interval in seconds (default: 2)\n\n"
              << "General Options:\n"
              << "  -v,    --version              Print version information and exit\n"
              << "  -h,    --help                 Print this help message and exit\n\n"
              << "Examples:\n"
              << "  " << (prog_name ? prog_name : "kronos") << " --port 8080 --target-port 9100\n"
              << "  " << (prog_name ? prog_name : "kronos") << " -p 9090 -t 192.168.1.50 -tp 9100 -i 5 -c 5000\n";
}

inline std::optional<AppConfig> parse_cli_args(int argc, char* argv[]) {
    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            config.show_help = true;
            return config;
        }
        if (arg == "-v" || arg == "--version") {
            config.show_version = true;
            return config;
        }

        auto require_value = [&](std::string_view flag_name) -> std::optional<std::string_view> {
            if (i + 1 < argc) {
                return std::string_view(argv[++i]);
            }
            std::cerr << "[CLI Error] Missing argument value for flag: " << flag_name << "\n";
            return std::nullopt;
        };

        if (arg == "-p" || arg == "--port") {
            auto val = require_value(arg);
            if (!val) return std::nullopt;
            unsigned int p = 0;
            auto [ptr, ec] = std::from_chars(val->data(), val->data() + val->size(), p);
            if (ec != std::errc{} || p == 0 || p > 65535) {
                std::cerr << "[CLI Error] Invalid port number: " << *val << "\n";
                return std::nullopt;
            }
            config.http_port = static_cast<std::uint16_t>(p);
        } else if (arg == "-t" || arg == "--target-host") {
            auto val = require_value(arg);
            if (!val) return std::nullopt;
            config.scrape_host = std::string(*val);
        } else if (arg == "-tp" || arg == "--target-port") {
            auto val = require_value(arg);
            if (!val) return std::nullopt;
            unsigned int p = 0;
            auto [ptr, ec] = std::from_chars(val->data(), val->data() + val->size(), p);
            if (ec != std::errc{} || p == 0 || p > 65535) {
                std::cerr << "[CLI Error] Invalid target port number: " << *val << "\n";
                return std::nullopt;
            }
            config.scrape_port = static_cast<std::uint16_t>(p);
        } else if (arg == "-path" || arg == "--target-path") {
            auto val = require_value(arg);
            if (!val) return std::nullopt;
            config.scrape_path = std::string(*val);
        } else if (arg == "-i" || arg == "--interval") {
            auto val = require_value(arg);
            if (!val) return std::nullopt;
            unsigned int sec = 0;
            auto [ptr, ec] = std::from_chars(val->data(), val->data() + val->size(), sec);
            if (ec != std::errc{} || sec == 0) {
                std::cerr << "[CLI Error] Invalid interval in seconds: " << *val << "\n";
                return std::nullopt;
            }
            config.scrape_interval = std::chrono::seconds{sec};
        } else if (arg == "-w" || arg == "--wal-path") {
            auto val = require_value(arg);
            if (!val) return std::nullopt;
            config.wal_path = std::filesystem::path(*val);
        } else if (arg == "-c" || arg == "--capacity") {
            auto val = require_value(arg);
            if (!val) return std::nullopt;
            std::size_t cap = 0;
            auto [ptr, ec] = std::from_chars(val->data(), val->data() + val->size(), cap);
            if (ec != std::errc{} || cap == 0) {
                std::cerr << "[CLI Error] Invalid capacity: " << *val << "\n";
                return std::nullopt;
            }
            config.storage_capacity = cap;
        } else {
            std::cerr << "[CLI Error] Unknown option: " << arg << "\n";
            std::cerr << "Run '" << (argv[0] ? argv[0] : "kronos") << " --help' for usage instructions.\n";
            return std::nullopt;
        }
    }

    return config;
}
