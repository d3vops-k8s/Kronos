#include <iostream>
#include "metric.h"

int main() {
    MetricPoint point;

    point.name = "node_cpu_seconds_total";
    point.value = 1245.67;
    point.timestamp = 1717200000;
    

    std::cout << "--- Kronos Metrics Point Test ---" << '\n';
    std::cout << "Name:        " << point.name << '\n';
    std::cout << "Value:       " << point.value << '\n';
    std::cout << "Timestamp:   " << point.timestamp << '\n';

    return 0;
}