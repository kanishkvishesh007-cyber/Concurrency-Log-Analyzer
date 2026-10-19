# Concurrency Log & Market Data Analyzer

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey.svg)]()

A high-throughput C++ concurrency workspace combining a **Lock-Free Data Processor** and a **Multi-Threaded Chunked Text Log Analyzer**.

This project demonstrates systems programming techniques in C++, including lock-free single-producer single-consumer (SPSC) queues, cache-line alignment to prevent false sharing, fine-grained std::atomic memory ordering, custom big-endian parsing, and parallel file chunking.

---

## 📌 Project Architecture

The repository contains two core high-performance concurrent processing engines:

### 1. Multi-Threaded Chunked Text Log Analyzer
* **Parallel Chunk Division**: Dynamically partitions large log files (`data.txt`) into distinct byte offsets across configured worker threads.
* **Line Realignment Handling**: Handles split-line boundaries across chunks cleanly.
* **Thread-Safe Metrics**: Uses atomic counters (`std::atomic<int>`) for lock-free aggregation of `ERROR`, `WARNING`, and `INFO` occurrences.
* **Regex Pattern Matching**: Evaluates log levels concurrently across independent thread streams.

### 2. Lock-Free Data Processor
* **Packet Capture Processing**: Uses **PcapPlusPlus** to extract UDP payload streams from raw `.pcap` files (`NasdaqTexasTotalView-ITCH5.0.pcap`).
* **Lock-Free SPSC Ring Buffer**: Implements an 8-slot SPSC queue with explicit memory ordering (`memory_order_relaxed`, `memory_order_acquire`, `memory_order_release`).
* **False Sharing Mitigation**: Employs `alignas(64)` on atomic head/tail pointers to isolate thread execution across distinct CPU cache lines.
* **Network Byte Order Conversion**: Custom bit-shift operations (`ret16`, `ret32`, `ret48`, `ret64`) utilizing `__builtin_bswap` intrinsics for efficient big-endian protocol unpacking.
* **Order Tracking**: Unpacks Add (`A`), Executed (`E`), and Delete (`D`) order messages, maintaining active order state and tracking executed funds.

---

## ⚡ Text Analyzer Thread Benchmarks

Below are real-world benchmark execution times recorded across thread count variations when processing the target log dataset:

* **Benchmark Date**: September 18, 2026
* **Target Environment**: Multi-Core Benchmark Host
* **Core Dataset**: Standard `data.txt` log payload

### Benchmark Results Table

| Thread Count | Execution Time (ms) | Speedup Factor | Performance Notes |
| :--- | :--- | :--- | :--- |
| **1 Thread** | `25,985.60 ms` | **1.00x** | Baseline single-threaded execution |
| **2 Threads** | `16,623.70 ms` | **1.56x** | Dual-core workload distribution |
| **4 Threads** | `8,623.72 ms` | **3.01x** | Near-linear scaling across physical cores |
| **8 Threads** | `5,095.10 ms` | **5.10x** | High parallel core utilization |
| **16 Threads** | `3,159.01 ms` | **8.23x** | Low resource contention |
| **32 Threads** | **`2,895.81 ms`** | **8.97x** | 🏆 **Optimal Peak Efficiency** |
| **64 Threads** | `3,355.68 ms` | **7.74x** | Context-switching & contention degradation |

### Performance Visualization

```text
 1 Thread  [==================================================] 25,985.6 ms
 2 Threads [=================================] 16,623.7 ms
 4 Threads [=================] 8,623.7 ms
 8 Threads [==========] 5,095.1 ms
16 Threads [======] 3,159.0 ms
32 Threads [=====] 2,895.8 ms  <-- PEAK PERFORMANCE (2.89s)
64 Threads [======] 3,355.7 ms  <-- CPU oversaturation / Thread overhead
```

### Insights
1. **Optimal Execution**: The framework reaches maximum throughput at **32 threads**, achieving an **8.97x speedup** compared to single-threaded parsing.
2. **Oversubscription Penalty**: Increasing thread count to 64 degrades performance back to `3,355.68 ms` due to CPU thread context-switching overhead and cache invalidations.

---

## 🛠️ Build and Prerequisites

### Requirements
* **Compiler**: Modern C++ compiler with C++17 support (`g++ >= 9.0`, `clang++ >= 10.0`, or MSVC 2019+)
* **Dependencies**:
  * [PcapPlusPlus](https://pcapplusplus.github.io/)
  * Libpcap / WinPcap / Npcap


## ✒️ Author

* **Kanishk Vishesh** - [*@kanishkvishesh007-cyber*](https://github.com/kanishkvishesh007-cyber)

---

## 📜 License

This project is open-source and available under the [MIT License](LICENSE).
