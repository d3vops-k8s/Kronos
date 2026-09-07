#include <iostream>
#include <fstream>
#include <string>
#include "parser.h"
#include "storage.h"

// Вспомогательная функция: красиво распечатать статус метрики
void print_metric_info(const InMemoryStorage& storage, const std::string& name) {
    auto latest = storage.get_latest(name);
    auto avg = storage.get_average(name);

    if (latest && avg) {
        std::cout << "[METRIC] " << name << '\n'
                  << "         Latest:  " << latest->value << '\n'
                  << "         Average: " << *avg << '\n';
    } else {
        std::cout << "[WARN]   " << name << " -> Not found!" << '\n';
    }
}

int main() {
    std::cout << "=== Kronos TSDB Engine ===\n\n";

    InMemoryStorage storage;
    std::ifstream file("data/metrics.txt");

    if (!file.is_open()) {
        std::cerr << "Error opening data/metrics.txt\n";
        return 1;
    }

    // Загрузка файла в память
    std::string line;
    while (std::getline(file, line)) {
        if (auto point = parse_line(line)) {
            storage.insert(*point);
        }
    }

    std::cout << "Tracked metrics: " << storage.metric_count() << "\n\n";

    // Опрос метрик (проверяем последнее значение и среднее)
    print_metric_info(storage, "node_cpu_seconds_total");
    print_metric_info(storage, "node_memory_MemTotal_bytes");
    print_metric_info(storage, "non_existent_metric");

    return 0;
}