#include <iostream>
#include <fstream>
#include <format>
#include <string>
#include <thread>
#include "parser.h"
#include "thread_safe_storage.h"

int main() {
    std::cout << std::format("=== Kronos TSDB Engine — Multithreaded Test (C++23) ===\n\n");


    ThreadSafeStorage storage;

    std::ifstream file("data/metrics.txt");
    if (!file.is_open()) {
        std::cerr << std::format("Error opening data/metrics.txt\n");
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (auto point = parse_line(line)) {
            storage.insert(*point);
        }
    }


    std::cout << std::format("Initial tracked metrics ({}):\n", storage.metric_count());
    for (const auto& name : storage.metric_names()) {
        std::cout << std::format("  -> {}\n", name);
    }
    std::cout << "\nStarting concurrent stress-test (Writer + Reader)...\n";

    constexpr int iterations = 100'000; 

    // RAII Scope: деструкторы jthread автоматически вызовут .join()
    // при закрытии фигурной скобки — даже если внутри вылетит исключение.
    // Это золотой стандарт C++23: никакого ручного .join() !
    {
        std::jthread writer([&storage]() {
            for (int i = 0; i < iterations; ++i) {
                MetricPoint point{
                    .name = "node_cpu_seconds_total",
                    .value = 100.0 + (i % 50),
                    .timestamp = 1717200000 + i
                };
                storage.insert(point);
            }
        });

        std::jthread reader([&storage]() {
            for (int i = 0; i < iterations; ++i) {
                auto latest = storage.get_latest("node_cpu_seconds_total");
                auto avg = storage.get_average("node_cpu_seconds_total");
                auto count = storage.metric_count();
                (void)latest;
                (void)avg;
                (void)count;
            }
        });

    } // <-- Здесь оба потока гарантированно завершились!

    std::cout << "\n=== Stress-test completed successfully! ===\n";
    auto latest = storage.get_latest("node_cpu_seconds_total");
    auto avg = storage.get_average("node_cpu_seconds_total");
    if (latest && avg) {
        std::cout << std::format("Metric: node_cpu_seconds_total\n");
        std::cout << std::format("  Latest Value: {}\n", latest->value);
        std::cout << std::format("  Average (RingBuffer): {:.2f}\n", *avg);
    }
    std::cout << std::format("Total active metrics in storage: {}\n", storage.metric_count());

    return 0;
}