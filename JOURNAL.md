# 📓 Kronos Engineering Journal

## [2026-09-04] — Day 1: Project Setup & Data Foundation
* **Concepts Learned:**
  * C++ Toolchain: `g++` compiler (MinGW-w64) and `CMake` build system.
  * Separation of concerns: Header files (`.h` — schemas/data blueprints) vs Implementation files (`.cpp`).
  * `#pragma once` directive — header guard against multiple inclusions.
  * Safe data types: `std::string`, `double`, and fixed-width `std::int64_t` from `<cstdint>`.
  * Default member initialization (`= 0`, `= 0.0`) to eliminate garbage memory values.
* **Project Milestones:**
  * Created project directory structure: `include/`, `src/`, `data/`.
  * Configured `.gitignore` to prevent build artifacts from polluting git.
  * Defined `MetricPoint` struct in `include/metric.h`.
  * Initialized Git repository, connected HTTPS remote, and pushed initial commit to GitHub.

---

## [2026-09-05] — Day 2: Build System, First Binary & Metric Parser
* **Concepts Learned:**
  * **CMake as Build Manifest:** C++20 standard, build targets (`add_executable`), and include paths (`target_include_directories`).
  * **IDE & Tooling:** Configured VS Code `clangd` via `.vscode/settings.json` (`--query-driver`) to enable clean diagnostics and autocompletion.
  * **Zero-Copy & Performance:** Value passing (`std::string`) vs pass-by-const-reference (`const std::string&`).
  * **Safe Error Handling:** `std::optional<MetricPoint>` pattern instead of null pointers or crashing.
  * **String Tokenization:** `std::stringstream` (analogous to `awk` в bash) for whitespace-delimited extraction.
  * **Standard Collections:** `std::vector` and range-based `for (const auto& item : vector)` loops.
* **Project Milestones:**
  * Built and executed the first native binary `kronos.exe`.
  * Adopted GitHub Flow: branched into `feat/parser`.
  * Declared parser interface in `include/parser.h`.
  * Implemented parser in `src/parser.cpp`: parsing metric name, value, optional timestamp, and safely ignoring `#` comments.
  * Validated parser behavior with mock test data in `src/main.cpp`.
  * Merged `feat/parser` into `main` and pushed to GitHub.

---

## [2026-09-06] — Day 3: File Ingestion & Ring Buffer Core (Completed)
* **Concepts Learned:**
  * **Cross-platform Portability:** How standard C++20 and CMake build identically on x86, ARM, Linux, Windows, and Docker.
  * **File I/O Streams:** `std::ifstream` from `<fstream>` and RAII automatic resource cleanup.
  * **Stream Safety:** Validating file states (`file.is_open()`), using `std::cerr` for errors, and standard POSIX exit codes.
  * **Ring Buffer & Modulo Arithmetic:** O(1) circular overwriting without heap allocations on write via `(head + 1) % capacity`.
  * **TCP Sliding Window Analogy:** Managing a bounded window of data points in memory to avoid OOM killer.
* **Project Milestones:**
  * Created `data/metrics.txt` and validated full file ingestion in `src/main.cpp`.
  * Implemented `RingBuffer` class (`include/ring_buffer.h` and `src/ring_buffer.cpp`).
  * Verified circular FIFO overwrite behavior in unit test.
  * Merged `feat/file-reader` and `feat/ring-buffer` into `main`.

---

## [2026-09-07] — Day 4: In-Memory Storage & Smart Pointers (Part 1 - Completed)
* **Concepts Learned:**
  * **Hash Tables in C++:** `std::unordered_map` for O(1) key-based routing by metric name.
  * **Smart Pointers & Ownership:** `std::unique_ptr<RingBuffer>` and `std::make_unique` to eliminate memory leaks and avoid expensive buffer copying.
  * **C++20 Idioms:** `storage_.contains()` for expressive key checks.
  * **End-to-End Pipeline Integration:** Streaming raw text from disk -> parsing OpenMetrics tokens -> dynamic routing into ring buffers -> querying by metric name.
* **Project Milestones:**
  * Created `InMemoryStorage` class (`include/storage.h` and `src/storage.cpp`).
  * Integrated full file-ingestion-to-storage pipeline in `src/main.cpp`.
  * Successfully queried CPU, memory, and non-existent metric paths with zero crashes.
  * Merged `feat/in-memory-storage` into `main`.
