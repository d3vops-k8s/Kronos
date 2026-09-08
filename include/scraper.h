#pragma once

#include <string>
#include <chrono>
#include <thread>
#include "thread_safe_storage.h"

// Конфигурация одного scrape-таргета.
// Аналог секции scrape_configs в prometheus.yml:
//   - job_name: 'node'
//     static_configs:
//       - targets: ['localhost:9100']
//     scrape_interval: 15s
struct ScrapeConfig {
    std::string host;                   // e.g. "localhost"
    int         port;                   // e.g. 9100
    std::string path{"/metrics"};       // e.g. "/metrics"
    std::chrono::seconds interval{15};  // Интервал скрейпа
};

// Scraper — фоновый HTTP-скрейпер.
//
// Жизненный цикл (аналог Kubernetes Pod):
//   scraper.start()  → Pod переходит в Running
//   scraper.stop()   → Pod получает SIGTERM (кооперативная остановка)
//   ~Scraper()       → jthread::~jthread() делает request_stop() + join() автоматически (RAII)
//
// Гарантия: деструктор НИКОГДА не завершится, пока фоновый поток ещё работает.
// Это полная противоположность std::thread, который при уничтожении без join() → std::terminate().
class Scraper {
public:
    explicit Scraper(ThreadSafeStorage& storage, ScrapeConfig config);

    // Запускает фоновый jthread (неблокирующий вызов).
    void start();

    // Запрашивает кооперативную остановку через stop_token (неблокирующий вызов).
    // Поток проснётся за ≤100ms и завершится сам.
    void stop();

private:
    // Главный цикл потока — получает stop_token автоматически от jthread
    void run(std::stop_token stop);

    // Выполняет один HTTP GET запрос и сохраняет распарсенные метрики в storage_
    void scrape_once();

    ThreadSafeStorage& storage_;  // Ссылка (не копия!): один storage на всю программу
    ScrapeConfig       config_;
    std::jthread       thread_;   // RAII: деструктор → request_stop() + join()
};
