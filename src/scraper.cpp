#include "scraper.h"
#include "parser.h"

#include <httplib.h>  // cpp-httplib: header-only HTTP/1.1 клиент и сервер
#include <iostream>
#include <format>
#include <sstream>

Scraper::Scraper(ThreadSafeStorage& storage, ScrapeConfig config)
    : storage_(storage), config_(std::move(config)) {}

void Scraper::start() {
    // std::jthread автоматически передаёт std::stop_token первым аргументом лямбды.
    // Нам не нужно вручную создавать stop_source — jthread делает это внутри.
    thread_ = std::jthread([this](std::stop_token stop) {
        run(stop);
    });
}

void Scraper::stop() {
    // Кооперативный сигнал остановки — аналог SIGTERM в Unix.
    // Поток сам проверяет stop.stop_requested() и завершается корректно.
    // В отличие от pthread_kill() или TerminateThread() — никакой гонки за ресурсы!
    thread_.request_stop();
}

void Scraper::run(std::stop_token stop) {
    std::cout << std::format("[Scraper] Started → target: {}:{}{}, interval: {}s\n",
        config_.host, config_.port, config_.path, config_.interval.count());

    while (!stop.stop_requested()) {
        scrape_once();

        // ⚠️ Антипаттерн (НЕ так):
        //     std::this_thread::sleep_for(config_.interval);
        // При shutdown поток будет спать всё время интервала, игнорируя сигнал остановки.
        //
        // ✅ Правильно: Прерываемое ожидание — дробим сон на кусочки по 100ms.
        // Аналог: Kubernetes terminationGracePeriodSeconds — даём поду время на завершение,
        // но проверяем готовность каждые 100ms.
        auto deadline = std::chrono::steady_clock::now() + config_.interval;
        while (!stop.stop_requested() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "[Scraper] Stopped gracefully.\n";
}

void Scraper::scrape_once() {
    // httplib::Client — аналог curl в C++.
    // Создаётся дёшево (нет постоянного соединения, как у curl --keepalive).
    httplib::Client client(config_.host, config_.port);
    client.set_connection_timeout(3);  // 3 секунды — стандартный продакшн таймаут

    auto result = client.Get(config_.path);

    if (!result) {
        // result.error() возвращает enum httplib::Error: Connection, Timeout, и т.д.
        std::cerr << std::format("[Scraper] ❌ Connection failed: {} → {}:{}{}\n",
            httplib::to_string(result.error()),
            config_.host, config_.port, config_.path);
        return;
    }

    if (result->status != 200) {
        std::cerr << std::format("[Scraper] ❌ HTTP {}: {}:{}{}\n",
            result->status, config_.host, config_.port, config_.path);
        return;
    }

    // Парсим тело ответа построчно — наш Parser уже умеет это делать!
    // std::istringstream разбивает строку на строки как cin, но из памяти (не с диска).
    std::istringstream stream(result->body);
    std::string line;
    int parsed_count = 0;

    while (std::getline(stream, line)) {
        if (auto point = parse_line(line)) {
            storage_.insert(*point);  // ThreadSafeStorage: unique_lock внутри
            ++parsed_count;
        }
    }

    std::cout << std::format("[Scraper] ✅ Scraped {} metrics from {}:{}{}\n",
        parsed_count, config_.host, config_.port, config_.path);
}
