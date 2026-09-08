// main.cpp — Интеграционный тест Спринта 3: Scraper + ThreadSafeStorage
//
// Запуск:
//   Терминал 1: .\build\mock_exporter.exe
//   Терминал 2: .\build\kronos.exe

#include <iostream>
#include <format>
#include <thread>
#include <chrono>
#include "thread_safe_storage.h"
#include "scraper.h"

using namespace std::chrono_literals;

int main() {
    std::cout << std::format("=== Kronos TSDB Engine — Sprint 3 Integration Test (C++23) ===\n\n");

    // Единственное хранилище для всей программы.
    // Скрапер будет писать в него через unique_lock,
    // репортер — читать через shared_lock. Никаких гонок!
    ThreadSafeStorage storage;

    // Конфигурация скрапера (аналог scrape_configs в prometheus.yml)
    ScrapeConfig config{
        .host     = "localhost",
        .port     = 9100,
        .path     = "/metrics",
        .interval = 5s   // Каждые 5 секунд — для наглядности теста
    };

    // Запускаем фоновый поток-скрапер
    Scraper scraper(storage, config);
    scraper.start();

    std::cout << "Scraper started. Collecting for 20 seconds (4 cycles × 5s)...\n";
    std::cout << "Make sure mock_exporter.exe is running in another terminal!\n\n";

    // Репортер — ещё один фоновый поток, который каждые 5 секунд
    // печатает снимок состояния хранилища (snapshot).
    // Демонстрируем Multiple Readers: reader и scraper работают параллельно без блокировок!
    {
        std::jthread reporter([&storage](std::stop_token stop) {
            int tick = 0;
            while (!stop.stop_requested()) {
                std::this_thread::sleep_for(5s);
                if (stop.stop_requested()) break;

                ++tick;
                std::cout << std::format("\n─── [Tick {}] Storage Snapshot ───────────────────\n", tick);
                std::cout << std::format("Active metrics: {}\n", storage.metric_count());

                for (const auto& name : storage.metric_names()) {
                    auto latest = storage.get_latest(name);
                    auto avg    = storage.get_average(name);
                    if (latest && avg) {
                        std::cout << std::format("  {:45s} latest={:>12.2f}  avg={:>12.2f}\n",
                            name, latest->value, *avg);
                    }
                }
            }
        });

        // Главный поток спит 20 секунд пока идёт сбор данных
        std::this_thread::sleep_for(20s);

    } // Деструктор reporter jthread: request_stop() + join() автоматически

    // Останавливаем скрапер
    scraper.stop();

    std::cout << "\n=== Sprint 3 complete! ===\n";
    std::cout << std::format("Final active metrics: {}\n", storage.metric_count());
    for (const auto& name : storage.metric_names()) {
        auto avg = storage.get_average(name);
        if (avg) std::cout << std::format("  {} → avg={:.2f}\n", name, *avg);
    }

    return 0;
}