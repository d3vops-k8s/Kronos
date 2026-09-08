#pragma once

#include <string>
#include <chrono>
#include <thread>
#include "thread_safe_storage.h"

// Configuration for a single scrape target, equivalent to a Prometheus scrape_config entry.
struct ScrapeConfig {
    std::string host;
    int         port;
    std::string path{"/metrics"};
    std::chrono::seconds interval{15};
};

// Background HTTP scraper.
//
// Runs a scrape loop in a dedicated std::jthread. On each iteration, it issues
// an HTTP GET request to the configured target, parses the OpenMetrics response,
// and inserts valid data points into the shared ThreadSafeStorage.
//
// Shutdown is cooperative via std::stop_token. The sleep between scrapes is
// implemented as a polling loop (100 ms ticks) so the thread responds to a
// stop request within 100 ms rather than waiting out the full interval.
//
// std::jthread guarantees join() is called automatically on destruction even
// if stop() is never called explicitly.
class Scraper {
public:
    explicit Scraper(ThreadSafeStorage& storage, ScrapeConfig config);

    // Launches the background scrape thread. Non-blocking.
    void start();

    // Requests cooperative shutdown. Non-blocking; returns before the thread exits.
    void stop();

private:
    void run(std::stop_token stop);
    void scrape_once();

    ThreadSafeStorage& storage_;
    ScrapeConfig       config_;
    std::jthread       thread_;
};
