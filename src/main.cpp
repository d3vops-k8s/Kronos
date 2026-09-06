#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "metric.h"
#include "parser.h"

int main() {
    const std::string file_path = "data/metrics.txt";

    std::ifstream file(file_path);

    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open file: " << file_path << '\n';
        return 1;
    }

    std::cout << "--- Kronos Metrics File Reader ---" << '\n';
    std::cout << "Reading from: " << file_path << "\n\n";

    std::vector<MetricPoint> parsed_metrics;
    std::string line;
    int total_lines = 0;
    int skipped_lines = 0;

    while (std::getline(file, line)) {
        total_lines++;

        auto result = parse_line(line);

        if (result.has_value()) {
            parsed_metrics.push_back(*result);
        } else {
            skipped_lines++;
        }
    }

    std::cout << "Successfully parsed metrics:" << '\n';
    for (const auto& metric : parsed_metrics) {
        std::cout << "   -> " << metric.name;
        std::cout << "    = " << metric.value;

        if (metric.timestamp != 0) {
            std::cout << " [ts: " << metric.timestamp << "]";
        }
        std::cout << '\n';
    }

    std::cout << "\n--- Summary ---" << '\n';
    std::cout << "Total lines read: " << total_lines << '\n';
    std::cout << "Valid metrcis:    " << parsed_metrics.size() << '\n';
    std::cout << "Skipped/Comments: " << skipped_lines << '\n';

    return 0;
}