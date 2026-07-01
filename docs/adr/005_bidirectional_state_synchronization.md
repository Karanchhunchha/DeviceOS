# ADR 005: Bidirectional State Synchronization

## Status
Approved

## Context
DeviceOS requires a way to apply desired state configurations sent from the cloud (desired shadow state) to the local C++ runtime. The parser must ingest JSON state changes dynamically, support multiple parameter types (integers, floats, strings, booleans), and operate without dynamic memory allocations (heap operations) on embedded targets.

## Decision
1. **Zero-Heap JSON Ingestion Scanner:** Implement a lightweight, zero-heap C++ string scanner inside `StateSync::onShadowPatch`. The parser scans the character array sequentially, extracts keys, detects value types (strings, digits, or booleans), and matches them to the static registers table.
2. **Synchronization State Alignment:** When a parameter value is updated via a cloud desired patch, the local state registry updates and clears its `dirty` flag (since the local reported now matches the cloud's desired value).
3. **Delta Ingestion Notification:** Upon applying each parameter change, the runtime emits a `STATE_SHADOW_DELTA_RECEIVED` event on the Event Bus containing the updated key name. This allows peripheral modules (e.g. GPIO relays, rule engines) to react to cloud command changes.

## Consequences
* **Positives:** Ingesting and parsing JSON updates consumes zero heap memory, protecting the microcontroller from out-of-memory crashes. Downstream modules react immediately via Event Bus callbacks.
* **Negatives:** The lightweight scanner does not support escape characters or nested JSON structures in the delta patch payload.
