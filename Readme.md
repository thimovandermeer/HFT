
---

# ⚡ Building a High-Frequency Trading System — An Experimental Journey

> This repository documents my exploration into building and optimizing a high-frequency trading (HFT) system from scratch — starting from a naive prototype and gradually evolving toward hardware-accelerated performance.
>
> It’s not about writing the fastest code right away. It’s about *learning what actually makes code fast* — by measuring, profiling, and experimenting at every layer of the stack.
>
> I’ll be following data — I’ll try to optimize based on what the profilers say instead of what I think should be optimized.

---

## 🧭 Motivation

The projects I’ve enjoyed most in my career were the ones where I had to **squeeze every last bit of performance out of the hardware** —
whether it was making **payment terminals** process transactions as fast as physically possible,
or getting the **brain of a self-driving robot** to run on a tiny **Arduino** with barely any resources.

That process — finding the limits of what hardware can do and bending them just a little further — is what I’ve always loved most.
This project is an extension of that passion: taking the same mindset and applying it to the extreme performance world of HFT.

---

## 🧩 Current Architecture (work-in-progress)

```mermaid
flowchart LR
    FeedSource(Bitvavo / FIXSim / Other) --> |WebSocket / TCP| QuotesObtainer
    QuotesObtainer --> OrderBook
    OrderBook --> TradingLogic
    TradingLogic --> |Buy/Sell| OrderSender
```

---

## 🧪 Phase 1 — Naive Implementation (Baseline)

> “First make it run.”

* Runs locally on macOS
* Focused purely on **functionality and structure**
* Uses standard STL containers (`std::vector`, `std::map`)
* Minimal threading, single market feed
* CLI visualization of order-book and trade flow
* Basic tick-to-trade latency logging

### 🎯 Goals

* Build a minimal but complete **end-to-end data flow**:
  **feed → order book → trading logic → simulated trade**
* Establish a baseline before optimizing anything
* Record first timing measurements (approximate but illustrative)

<details>
<summary>🧾 Reflection (to be filled after completion)</summary>

* What worked well in the architecture?
* Where did latency first appear?
* Did the system behave predictably under load?
* What surprised me?

</details>

---

## 🧰 Phase 2 — Dedicated Hardware Setup (Ubuntu Environment)

* Move to **dedicated Linux hardware** for reproducible, controllable performance testing.
* Set up the machine, install dependencies, and prepare a stable profiling environment.
* No tuning yet — focus on reproducibility and determinism.

### Goals

* Stable environment for performance testing
* Automated benchmarking setup
* First reproducible latency measurements under Linux

<details>
<summary>🧾 Reflection (to be filled after completion)</summary>

* How reproducible are the results vs macOS?
* What configuration challenges appeared?
* Which parts of the setup most impacted stability?

</details>

---

## 🔍 Phase 3 — Software Optimization (Pre-Hardware Tuning)

* Optimize software performance **before** touching hardware parameters.
* Focus on code-level and algorithmic improvements.

### Tasks

* Profile baseline with `perf` and FlameGraphs
* Identify CPU hot paths
* Replace STL containers with cache-friendly alternatives (`tsl::robin_map`, `boost::flat_map`)
* Reduce dynamic allocations (custom allocators, arena pools)
* Introduce lock-free SPSC queue for feed → order book
* Simplify parsing logic
* Optimize threading model
* Compare latency before and after optimizations

<details>
<summary>🧾 Reflection (to be filled after completion)</summary>

* Which optimizations provided real gains?
* Which looked promising but didn’t matter?
* How predictable are the improvements?

</details>

---

## 🌐 Phase 4 — Raw Network I/O (UDP & Binary Feed Handling)

* Transition from high-level WebSocket connections to **raw UDP byte streams**.
* Implement direct binary feed handling to understand the full network stack and measure wire-to-application latency.

### Tasks

* Build a lightweight UDP client for quote reception
* Parse raw binary messages instead of JSON/WebSocket
* Benchmark parsing performance vs WebSocket layer
* Analyze packet drops, kernel buffers, and syscalls
* Experiment with kernel-bypass networking (DPDK / AF_XDP)
* Measure the difference in latency and jitter
* Implement packet sequence tracking and loss detection
* Integrate UDP feed into the main pipeline

<details>
<summary>🧾 Reflection (to be filled after completion)</summary>

* What was the real latency gain vs WebSockets?
* Which part of the stack became the new bottleneck?
* How complex did error handling and reassembly become?
* What did I learn about network stack internals?

</details>

---

## 🧮 Phase 5 — Custom Data Structures & Memory Management

* Implement **bespoke components** to replace off-the-shelf libraries.
* Focus on deep understanding and control over data movement.

### Tasks

* Implement custom lock-free queue
* Implement simple allocator or memory arena
* Implement fixed-capacity order book container
* Explore cache-friendly layouts (SoA vs AoS)
* Add micro-benchmarks for each structure
* Measure latency impact vs Boost/TSL alternatives

<details>
<summary>🧾 Reflection (to be filled after completion)</summary>

* How much improvement did custom data structures yield?
* What trade-offs in complexity vs performance?
* What lessons learned from lock-free and memory models?

</details>

---

## ⚙️ Phase 6 — Hardware-Level Tuning & Profiling

* Once the software path is fully optimized, begin hardware-level tuning.

### Tasks

* Configure BIOS and kernel for performance
* Set IRQ affinity and core isolation
* Enable hugepages and NUMA pinning
* Test thread pinning with `taskset`
* Measure jitter and tail latency under load
* Compare tuned vs untuned system metrics

<details>
<summary>🧾 Reflection (to be filled after completion)</summary>

* Which tuning had the biggest impact?
* How deterministic are results after pinning and isolation?
* What unexpected interactions appeared between OS and code?

</details>

---

## 🧱 Phase 7 — Hardware Acceleration (FPGA Exploration)

* Explore offloading components to FPGA for deterministic ultra-low latency.
* Implement minimal viable hardware modules and measure real-world performance.

### Tasks

* Set up FPGA toolchain (Quartus / Vivado)
* Implement UDP packet parser in Verilog/VHDL
* Integrate DMA with host process
* Compare FPGA vs CPU performance
* Document full software-hardware data path

<details>
<summary>🧾 Reflection (to be filled after completion)</summary>

* Which components benefited most from hardware?
* What are the complexity trade-offs?
* How close is hardware performance to theoretical limits?

</details>

---

## 📈 Progress Tracking (to be updated)

| Phase                      | Avg Tick-to-Trade (µs) | 99th Percentile (µs) | Notes                      |
| :------------------------- | ---------------------: | -------------------: | :------------------------- |
| 1 – Naive                  |                    TBD |                  TBD | macOS baseline             |
| 2 – Ubuntu setup           |                    TBD |                  TBD | Stable Linux environment   |
| 3 – Software optimized     |                    TBD |                  TBD | Algorithmic improvements   |
| 4 – Raw UDP feed           |                    TBD |                  TBD | Binary feed implementation |
| 5 – Custom data structures |                    TBD |                  TBD | Own lock-free / allocator  |
| 6 – Hardware tuned         |                    TBD |                  TBD | Core isolation, NUMA       |
| 7 – FPGA                   |                    TBD |                  TBD | Hardware offload           |

---

## 🧰 Toolchain

* **C++20**, CMake, Conan
* **Profilers:** `perf`, FlameGraph, VTune
* **Libraries:** Boost, fmt, spdlog
* **Network Frameworks:** DPDK / AF_XDP
* **Hardware:** Mellanox NIC, DE10-Nano FPGA
* **Visualization:** CLI dashboard, Grafana metrics

---

## 📚 Learning Objectives

* Understand the full stack of low-latency systems
* Learn to measure rather than guess
* Build intuition for caches, NUMA, and scheduling
* Explore kernel-bypass networking
* Implement and reason about lock-free data structures
* Work directly with binary network feeds and UDP
* Gain hands-on experience with hardware acceleration

---

## 💬 Philosophy

> “Measure first, optimize second, automate third.”

This is an **open-ended experiment**, not a predefined tutorial.
The direction may change at any point if the data shows something unexpected

---



