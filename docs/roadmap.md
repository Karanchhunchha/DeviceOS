# DeviceOS Monorepo Roadmap & Phase Tracking

This document outlines the active project roadmap, tracking milestones across runtime edge components and cloud backends.

---

## 1. Roadmap Phase Overview

### Phase 1: Foundation (Months 1-3)
* [x] **Step 1:** HAL interface architectures (IGPIO, INetwork).
* [x] **Step 2:** Runtime Kernel, Event Bus, State Machine, and Cooperative Scheduler.
* [x] **Step 3:** Local circular offline telemetry cache.
* [x] **Step 4:** Integrated bootstrap verification tests.

### Phase 2: Cloud Sync (Months 4-6)
* [x] **Step 1:** Go-based Device Registry, Shadow HTTP APIs, and `deviceos` developer CLI. (Completed)
* [x] **Step 2:** C++ Runtime network driver mapping and secure shadows payload ingestion. (Completed)
* [x] **Step 3:** State synchronization delta loops (calculating cloud delta updates on the device and publishing reported status). (Completed)
* [x] **Step 4:** Real-time WebSocket console connections. (Completed)

### Phase 3: Rollouts & Management (Months 7-9)
* [x] **Step 1:** Dual-partition secure boot OTA updater on ESP32 runtime. (Completed)
* [ ] **Step 2:** Next.js dashboard console integration.
* [ ] **Step 3:** Advanced rule engine validation.

---

## 2. Active Milestone Status
* **Current Step:** Phase 3 - Step 1 (Completed).
* **Next Target:** Phase 3 - Step 2 (Next.js dashboard console integration).
