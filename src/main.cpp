#include <iostream>
#include <vector>
#include "parser.h"

int main() {
    std::vector<std::string> test_lines = {
        "node_cpu_seconds_total 1245.67",
        "# HELP node_cpu_seconds_total Total CPU time",
        "node_memory_Active_bytes 4194304000 17172000000",
        "broken_metrics_without_value"
    };

    std::cout << "--- Testing Kronos Parser ---" << '\n';

    for (const std::string& line : test_lines) {
        auto result = parse_line(line);

        if (result.has_value()) {
            std::cout << "[OK] Parsed: " << result->name
                      << " = " << result->value
                      << " (timestamp: " << result->timestamp << ")" << '\n';
        } else {
            std::cout << "[SKIP/ERROR] Ignored line: " << line << '\n';
        }
    }

    return 0;
}