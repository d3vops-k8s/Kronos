#include <iostream>
#include <fstream>
#include <format>
#include <string>
#include "parser.h"
#include "storage.h"

void print_metric_info(const InMemoryStorage& storage, const std::string& name) {
    auto latest = storage.get_latest(name);
    auto avg = storage.get_average(name);

    if (latest && avg) {
        std::cout << std::format("[METRIC] {}\n", name);
        std::cout << std::format("         Latest:  {}\n", latest->value);
        std::cout << std::format("         Average: {}\n", *avg);
    } else {
        std::cout << std::format("[WARN]   {} -> Not found!\n", name);
    }
}

int main() {
    std::cout << std::format("=== Kronos TSDB Engine (C++23) ===\n\n");

    InMemoryStorage storage;
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

    std::cout << std::format("Tracked metrics: {}\n\n", storage.metric_count());

    print_metric_info(storage, "node_cpu_seconds_total");
    print_metric_info(storage, "node_memory_MemTotal_bytes");
    print_metric_info(storage, "non_existent_metric");

    return 0;
}