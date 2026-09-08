cmake_minimum_required(VERSION 3.20)

project(Kronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Зависимости ──────────────────────────────────────────────────────────────
# cpp-httplib: header-only HTTP/1.1 клиент+сервер, ~400KB, без внешних DLL.
# FetchContent автоматически скачивает и делает заголовки доступными через CMake target.
# Аналог: pip install в Python, npm install в Node — но встроенный в систему сборки.
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.20.0
)

# HTTPLIB_REQUIRE_OPENSSL=OFF: используем plain HTTP (без TLS).
# На production будет ON — для HTTPS скрейпинга.
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(httplib)

# ─── Основной бинарник ────────────────────────────────────────────────────────
add_executable(kronos
    src/main.cpp
    src/parser.cpp
    src/ring_buffer.cpp
    src/storage.cpp
    src/thread_safe_storage.cpp
    src/scraper.cpp
)

target_include_directories(kronos PRIVATE include)
target_link_libraries(kronos PRIVATE httplib::httplib)

# ─── Mock Exporter (тестовый HTTP сервер) ─────────────────────────────────────
add_executable(mock_exporter
    tests/mock_exporter.cpp
)

target_include_directories(mock_exporter PRIVATE include)
target_link_libraries(mock_exporter PRIVATE httplib::httplib)