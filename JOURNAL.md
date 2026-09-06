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
  * **String Tokenization:** `std::stringstream` (analogous to `awk` in bash) for whitespace-delimited extraction.
  * **Standard Collections:** `std::vector` and range-based `for (const auto& item : vector)` loops.
* **Project Milestones:**
  * Built and executed the first native binary `kronos.exe`.
  * Adopted GitHub Flow: branched into `feat/parser`.
  * Declared parser interface in `include/parser.h`.
  * Implemented parser in `src/parser.cpp`: parsing metric name, value, optional timestamp, and safely ignoring `#` comments.
  * Validated parser behavior with mock test data in `src/main.cpp`.
  * Merged `feat/parser` into `main` and pushed to GitHub.

---

## [2026-09-06] — Day 3: File Ingestion & CLI Validation (In Progress)
* **Objective:** Read real OpenMetrics lines from a disk file (`data/metrics.txt`), parse them into `std::vector<MetricPoint>`, and render a structured CLI summary report.
