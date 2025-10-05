# 📋 TODO — HFT Experimental Journey

## Phase 1 — Naive Implementation
- [X] Implement `QuotesObtainer`
- [X] Implement `OrderBook`
- [ ] Implement `TradingLogic`
- [ ] Implement `OrderSender`
- [ ] Implement `Vizualizer`
- [ ] Connect full pipeline: **feed → order book → trading logic → trade**
- [ ] Add latency and throughput logging
- [ ] Document baseline results

---

## Phase 2 — Dedicated Hardware Setup
- [ ] Select and purchase hardware
- [ ] Assemble and install Ubuntu
- [ ] Install `build-essential`, `cmake`, `clang`, `perf`
- [ ] Install Conan + Boost toolchain
- [ ] Migrate project to Linux
- [ ] Verify reproducible output
- [ ] Document setup and specs

---

## Phase 3 — Software Optimization
- [ ] Run `perf record` and analyze hot paths
- [ ] Generate FlameGraphs
- [ ] Replace STL containers with cache-friendly ones
- [ ] Add memory pooling / allocators
- [ ] Introduce lock-free SPSC queue
- [ ] Simplify quote parsing
- [ ] Tune threading model
- [ ] Measure and document improvements

---

## Phase 4 — Custom Data Structures
- [ ] Implement own lock-free queue
- [ ] Implement simple allocator
- [ ] Implement fixed-capacity order book container
- [ ] Explore SoA vs AoS memory layout
- [ ] Add microbenchmarks for each component
- [ ] Compare performance to Boost/TSL versions
- [ ] Document insights and trade-offs

---

## Phase 5 — Hardware-Level Tuning & Profiling
- [ ] Configure BIOS for performance mode
- [ ] Disable hyper-threading and power saving
- [ ] Configure `isolcpus`, `nohz_full`, IRQ affinities
- [ ] Enable hugepa
