#pragma once

#include <cstdint>
#include <httplib.h>
#include "thread_safe_storage.h"

// Embedded HTTP server for metrics querying and web dashboard.
//
// Endpoints:
//   GET /                        - Web dashboard (HTML/JS)
//   GET /health                  - Liveness probe: {"status":"ok"}
//   GET /api/v1/metrics_list     - JSON array of tracked metric names
//   GET /api/v1/query?name=...   - JSON details for a specific metric
class HttpServer {
public:
    // storage is held by reference; must outlive the HttpServer instance.
    explicit HttpServer(ThreadSafeStorage& storage, std::uint16_t port = 8080);

    // Starts the HTTP server (blocking call, typically run on the main thread).
    void start();

    // Stops the server and unblocks start().
    void stop();

private:
    void register_routes();

    void handle_index(const httplib::Request& req, httplib::Response& res);
    void handle_health(const httplib::Request& req, httplib::Response& res);
    void handle_metrics_list(const httplib::Request& req, httplib::Response& res);
    void handle_query(const httplib::Request& req, httplib::Response& res);

    ThreadSafeStorage& storage_;
    httplib::Server    server_;
    std::uint16_t      port_;
};