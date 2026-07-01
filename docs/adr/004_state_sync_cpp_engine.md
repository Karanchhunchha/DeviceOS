# ADR 004: C++ State Manager & Sync Engine Architecture

## Status
Approved

## Context
The DeviceOS C++ SDK needs to manage local parameters (integers, floats, strings) and synchronise them with the Cloud Shadow Service. The implementation must operate on low-power, constrained microcontrollers without dynamic heap allocation to prevent memory fragmentation and heap leaks.

## Decision
1. **Static Table Design:** Maintain a static array of state keys (`StateRegister`) with pre-allocated limits:
   * Max registers: 16
   * Max key length: 32 bytes
   * Max string value size: 64 bytes
2. **Dirty Flags Sweep:** Track modifications with a `dirty` flag per register. Sync sweeps only process dirty values.
3. **Placement-New Construction:** Use placement-new operators inside `DeviceCore` constructor to instantiate the `StateSync` and `OfflineCache` engines into static buffers. This avoids initialization order dependencies while ensuring zero heap usage.
4. **Decoupled Event-Driven Fallbacks:** The `StateSync` sweeps serialize dirty values and publish them to the `EventBus` when offline. The `OfflineCache` intercepts these telemetry events (IDs `0x3000` to `0x300F`) and commits them to virtual flash partitions automatically.

## Consequences
* **Positives:** RAM overhead is fixed (~1.6 KB). Heap allocations are zero. The synchronization flow is decoupled using Event Bus publications.
* **Negatives:** Adding more than 16 system state parameters will trigger `OUT_OF_MEMORY` errors unless the static limit is increased.
