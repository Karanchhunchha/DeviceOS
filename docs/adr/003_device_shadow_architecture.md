# ADR 003: Device Shadow Engine & Registry Architecture

## Status
Approved

## Context
DeviceOS requires a cloud-side state synchronization engine (Device Shadow) and a device registry. The system must support scaling to thousands of concurrent devices while remaining extremely easy for developers to deploy locally for testing and debugging.

## Decision
1. **Language Platform:** Implement the services in Go to leverage its low runtime memory footprint (<15MB idle), fast boot profiles (<20ms), and excellent concurrent networking primitives.
2. **Database Persistence:** Write standard, ANSI-compliant SQL targeting PostgreSQL. However, for local compilation and development convenience, utilize the pure-Go SQLite driver (`github.com/glebarez/go-sqlite`), which compiles without CGO or external dependencies.
3. **AWS IoT-Style Shadows:** Enforce state sync documents containing `reported` and `desired` states, calculating delta objects on updates:
   $$\Delta = \text{desired} \setminus \text{reported}$$
4. **Recursive Delta:** Implement recursive JSON comparison to support deep-nested configurations (e.g. nested parameter overrides).

## Consequences
* **Positives:** Developers can boot the entire cloud registry locally with a single command and zero environment dependencies. Memory footprint remains highly optimized, and database tables are 100% compatible with production PostgreSQL.
* **Negatives:** SQLite lock contentions during heavy concurrent writes. For production loads, the driver pool must point to high-throughput PostgreSQL or Spanner nodes.
