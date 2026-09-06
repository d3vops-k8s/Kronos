#include <iostream>
#include "metric.h"
#include "ring_buffer.h"

int main() {
    std::cout << "--- Testing Kronos RingBuffer ---" << "\n\n";

    const std::size_t capacity = 3;
    RingBuffer ring(capacity);

    std::cout << "Initial state: size = " << ring.size() 
              << ", capacity = " << ring.capacity() 
              << ", is_empty = " << (ring.empty() ? "true" : "false") << "\n\n";

    std::cout << "Pushing: 10.0, 20.0, 30.0..." << '\n';
    ring.push({"node_cpu_seconds_total", 10.0, 1001});
    ring.push({"node_cpu_seconds_total", 20.0, 1002});
    ring.push({"node_cpu_seconds_total", 30.0, 1003});

    std::cout << "Current size: " << ring.size() << " / " << ring.capacity() << '\n';
    if (auto latest = ring.get_latest()) {
        std::cout << "Latest metric: " << latest->name << " = " << latest->value << '\n';
    }

    std::cout << "\nPushing 4th point: 40.0 (buffer is full!)..." << '\n';
    ring.push({"node_cpu_seconds_total", 40.0, 1004});

    std::cout << "Size after overflow: " << ring.size() << " / " << ring.capacity() << '\n';
    if (auto latest = ring.get_latest()) {
        std::cout << "Latest metric now: " << latest->name << " = " << latest->value << '\n';
    }

    return 0;
}