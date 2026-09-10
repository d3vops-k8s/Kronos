cmake_minimum_required(VERSION 3.20)

project(Kronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Required by clangd for IDE diagnostics.

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
# cpp-httplib: header-only HTTP/1.1 client and server (~400 KB, no shared libs).
# FetchContent downloads and makes the CMake target available at configure time.
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.20.0
)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)  # Plain HTTP only; enable for TLS.
FetchContent_MakeAvailable(httplib)

# ---------------------------------------------------------------------------
# Main binary
# ---------------------------------------------------------------------------
add_executable(kronos
    src/main.cpp
    src/parser.cpp
    src/ring_buffer.cpp
    src/storage.cpp
    src/thread_safe_storage.cpp
    src/scraper.cpp
    src/http_server.cpp
    src/wal.cpp
    src/bit_stream.cpp
    src/gorilla.cpp
)

target_include_directories(kronos PRIVATE include)
target_link_libraries(kronos PRIVATE httplib::httplib)

# ---------------------------------------------------------------------------
# Mock exporter (integration test helper)
# ---------------------------------------------------------------------------
add_executable(mock_exporter
    tests/mock_exporter.cpp
)

target_include_directories(mock_exporter PRIVATE include)
target_link_libraries(mock_exporter PRIVATE httplib::httplib)

add_executable(benchmark_gorilla
    tests/benchmark_gorilla.cpp
    src/bit_stream.cpp
    src/gorilla.cpp
)
target_include_directories(benchmark_gorilla PRIVATE include)