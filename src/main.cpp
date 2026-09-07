#include <iostream>
#include <fstream>
#include <string>
#include "parser.h"
#include "storage.h"

int main() {
    const std::string file_path = "data/metrics.txt";
    std::ifstream file(file_path);

    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open file: " << file_path << '\n';
        return 1;
    }

    std::cout << "=== Kronos Time-Series In-Memory Engine ===" << "\n\n";
    std::cout << "Loading metrics from: " << file_path << "...\n";

    InMemoryStorage storage(1000);

    std::string line;
    int ingested_count = 0;

    while (std::getline(file, line)) {
        auto point = parse_line(line);
        if (point.has_value()) {
            storage.insert(*point);
            ingested_count++;
        }
    }

    std::cout << "Ingestion complete!\n";
    std::cout << "Total points ingested:   " << ingested_count << '\n';
    std::cout << "Unique metrics tracked:  " << storage.metric_count() << "\n\n";

    std::cout << "--- Querying Metrics by Name ---" << '\n';

    if (auto cpu = storage.get_latest("node_cpu_seconds_total")) {
        std::cout << "[FOUND] " << cpu->name << " = " << cpu->value 
                  << " (timestamp: " << cpu->timestamp << ")" << '\n';
    } else {
        std::cout << "[NOT FOUND] node_cpu_seconds_total" << '\n';
    }

    if (auto mem = storage.get_latest("node_memory_MemTotal_bytes")) {
        std::cout << "[FOUND] " << mem->name << " = " << mem->value 
                  << " (timestamp: " << mem->timestamp << ")" << '\n';
    } else {
        std::cout << "[NOT FOUND] node_memory_MemTotal_bytes" << '\n';
    }

    if (auto fake = storage.get_latest("non_existent_metric")) {
        std::cout << "[FOUND] " << fake->name << " = " << fake->value << '\n';
    } else {
        std::cout << "[SAFE]  non_existent_metric -> Correctly reported as NOT FOUND!" << '\n';
    }

    return 0;
}