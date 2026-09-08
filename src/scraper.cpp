#include "scraper.h"
#include "parser.h"

#include <httplib.h>
#include <iostream>
#include <format>
#include <sstream>

Scraper::Scraper(ThreadSafeStorage& storage, ScrapeConfig config)
    : storage_(storage), config_(std::move(config)) {}

void Scraper::start() {
    // std::jthread forwards a stop_token as the first lambda argument automatically.
    thread_ = std::jthread([this](std::stop_token stop) {
        run(stop);
    });
}

void Scraper::stop() {
    thread_.request_stop();
}

void Scraper::run(std::stop_token stop) {
    std::cout << std::format("[Scraper] started → {}:{}{} every {}s\n",
        config_.host, config_.port, config_.path, config_.interval.count());

    while (!stop.stop_requested()) {
        scrape_once();

        // Interruptible wait: sleep in 100 ms increments so a stop request is
        // honoured promptly rather than after the full scrape interval.
        auto deadline = std::chrono::steady_clock::now() + config_.interval;
        while (!stop.stop_requested() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "[Scraper] stopped\n";
}

void Scraper::scrape_once() {
    httplib::Client client(config_.host, config_.port);
    client.set_connection_timeout(3);

    auto result = client.Get(config_.path);
    if (!result) {
        std::cerr << std::format("[Scraper] connection error: {} ({}:{}{})\n",
            httplib::to_string(result.error()),
            config_.host, config_.port, config_.path);
        return;
    }

    if (result->status != 200) {
        std::cerr << std::format("[Scraper] HTTP {} from {}:{}{}\n",
            result->status, config_.host, config_.port, config_.path);
        return;
    }

    // Parse the response body line by line and ingest valid points into storage.
    std::istringstream stream(result->body);
    std::string line;
    int count = 0;

    while (std::getline(stream, line)) {
        if (auto point = parse_line(line)) {
            storage_.insert(*point);
            ++count;
        }
    }

    std::cout << std::format("[Scraper] scraped {} metrics from {}:{}{}\n",
        count, config_.host, config_.port, config_.path);
}
