// Kronos TSDB — Sprint 3 integration test.
//
// Starts a Scraper that polls mock_exporter every 5 seconds and a Reporter
// thread that prints a storage snapshot every 5 seconds for 20 seconds.
//
// Prerequisites:
//   Start mock_exporter.exe in a separate terminal before running this binary.

#include <iostream>
#include <format>
#include <thread>
#include <chrono>
#include "thread_safe_storage.h"
#include "scraper.h"

using namespace std::chrono_literals;

int main() {
    std::cout << "=== Kronos TSDB — Sprint 3 Integration Test ===\n\n";

    ThreadSafeStorage storage;

    ScrapeConfig config{
        .host     = "localhost",
        .port     = 9100,
        .path     = "/metrics",
        .interval = 5s,
    };

    Scraper scraper(storage, config);
    scraper.start();

    std::cout << "Collecting for 20 seconds (requires mock_exporter.exe on :9100)...\n\n";

    // Reporter thread: prints a storage snapshot every 5 seconds.
    // Demonstrates concurrent reads alongside the scraper's writes.
    {
        std::jthread reporter([&storage](std::stop_token stop) {
            int tick = 0;
            while (!stop.stop_requested()) {
                std::this_thread::sleep_for(5s);
                if (stop.stop_requested()) break;

                std::cout << std::format("\n--- snapshot [tick {}] ---\n", ++tick);
                std::cout << std::format("metrics tracked: {}\n", storage.metric_count());

                for (const auto& name : storage.metric_names()) {
                    auto latest = storage.get_latest(name);
                    auto avg    = storage.get_average(name);
                    if (latest && avg) {
                        std::cout << std::format("  {:45s}  latest={:>12.2f}  avg={:>12.2f}\n",
                            name, latest->value, *avg);
                    }
                }
            }
        });

        std::this_thread::sleep_for(20s);
    } // reporter jthread destructor: request_stop() + join()

    scraper.stop();

    std::cout << "\n=== done ===\n";
    std::cout << std::format("final metric count: {}\n", storage.metric_count());

    return 0;
}