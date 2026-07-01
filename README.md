<div align="center">

# DeviceOS

**An IoT Runtime, Cloud Platform, Simulator, and Companion App**  
*Build, test, and manage connected devices — without hardware dependencies.*

[![Version](https://img.shields.io/badge/version-v0.1.0--alpha-blue?style=flat-square)](https://github.com/karanchhunchha/DeviceOS/releases)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)
[![Go](https://img.shields.io/badge/Go-1.22-00ADD8?style=flat-square&logo=go)](https://golang.org)
[![Next.js](https://img.shields.io/badge/Next.js-15-black?style=flat-square&logo=next.js)](https://nextjs.org)
[![Status](https://img.shields.io/badge/status-active--development-orange?style=flat-square)]()

</div>

---

> **Status:** Active Development (Alpha)  
> DeviceOS is an experimental IoT platform under active development. Core runtime, simulator, cloud backend, and dashboard are functional. Flutter companion app and real hardware support are in progress.

## What is DeviceOS?

DeviceOS is a production-grade, hardware-independent IoT platform. It allows developers to **design, simulate, provision, monitor, and manage connected devices** before purchasing any physical hardware.

The platform is built around a core architectural principle:

> **Hardware is just another transport implementation.**

Everything from the provisioning flow, device shadow, telemetry pipeline, and OTA engine runs identically whether backed by a software simulator today or an ESP32 tomorrow.

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│                  Flutter App                    │  (Companion)
└──────────────────────┬──────────────────────────┘
                       │ BLE GATT / HTTP (simulated)
┌──────────────────────▼──────────────────────────┐
│            ProvisioningTransport API            │  (Abstraction Layer)
└──────────────────────┬──────────────────────────┘
                       │
         ┌─────────────┴─────────────┐
         │                           │
┌────────▼────────┐       ┌──────────▼──────────┐
│  Simulator      │       │  ESP32 / Real HW     │
│  (HTTP local)   │       │  (BLE GATT)          │
└────────┬────────┘       └──────────┬──────────┘
         │                           │
         └─────────────┬─────────────┘
                       │ Wi-Fi / MQTT
┌──────────────────────▼──────────────────────────┐
│              Go Cloud Platform                  │
│  Device Registry · Shadow API · WebSocket Hub   │
└──────────────────────┬──────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────┐
│             Next.js Dashboard                   │
│  Fleet View · Provisioning · Live Telemetry     │
└─────────────────────────────────────────────────┘
```

---

## Components

| Component | Technology | Status |
|-----------|-----------|--------|
| C++ Runtime | C++17, CMake | ✅ Complete |
| Hardware Abstraction Layer | C++ Interfaces | ✅ Complete |
| Device Simulator | Go | ✅ Complete |
| Cloud Platform | Go, SQLite | ✅ Complete |
| Dashboard | Next.js 15, React | ✅ Complete |
| Provisioning Flow (E2E) | HTTP → State Machine | ✅ Complete |
| Device Shadow API | REST | ✅ Complete |
| OTA Engine (Core) | C++ | ✅ Complete |
| Automated Test Suite | Go test, Node.js | ✅ 18/18 passing |
| Flutter Companion App | Flutter 3 | 🚧 In Progress |
| CI/CD Pipeline | GitHub Actions | 🔜 Planned |
| Digital Twin | — | 🔜 Planned |

---

## Quick Start

### Prerequisites

- Go 1.22+
- Node.js 20+
- CMake 3.20+ (for C++ runtime)

### 1. Start the Cloud

```bash
go run ./cloud/cmd/server
# → Listening on :8080
```

### 2. Start the Dashboard

```bash
cd dashboard
npm install
npm run dev
# → http://localhost:3000
```

### 3. Run the Simulator

```bash
go run ./cli/cmd/deviceos simulate --devices 3
# → sim_dev_0001 advertising on port 9001
# → sim_dev_0002 advertising on port 9002
# → sim_dev_0003 advertising on port 9003
```

### 4. Provision a Device

Open `http://localhost:3000` → **BLE Provisioning** → Enter port `9001` and credentials → Click **Provision Device**.

Watch the state machine execute:

```
BLE_ADVERTISING → PROVISIONING → [BLE OFF] → WIFI_CONNECTING
 → CLOUD_CONNECTING → SHADOW_SYNC → RUNNING
```

---

## Provisioning State Machine

```
UNPROVISIONED
     │
BLE_ADVERTISING          ← HTTP server starts on port 900X (simulates BLE GATT)
     │
PROVISIONING             ← Waiting for Wi-Fi credentials from dashboard/app
     │
[StopAdvertising]        ← BLE intentionally shut down by FSM (not a crash)
     │
WIFI_CONNECTING          ← Simulates Wi-Fi join delay
     │
CLOUD_CONNECTING         ← Registers device with cloud registry
     │
SHADOW_SYNC              ← Pulls initial desired state
     │
RUNNING                  ← Telemetry loop starts, publishes every 3-6s
```

---

## Automated QA

Run the full 10-test end-to-end suite:

```bash
# With all 3 services running:
node qa_test.js
```

**Results: 18/18 tests passing**

```
✅ Cloud :8080 reachable
✅ Dashboard :3000 serving
✅ Simulator ports 9001/9002/9003 listening
✅ CORS preflight returns correct headers
✅ Provision device → 200 OK
✅ Cloud shadow created after provisioning
✅ All 3 devices registered in cloud
✅ Wrong port → connection refused (expected)
✅ Invalid JSON → 400 (expected)
✅ Non-existent device → 404 (expected)
✅ Live telemetry: temperature changing between polls
```

---

## Failure Resilience

The simulator models realistic failure conditions:

| Failure | Behavior |
|---------|----------|
| Wi-Fi disconnect | Auto-reconnects via `CLOUD_CONNECTING` |
| Packet loss | Telemetry dropped, loop continues |
| Power cycle | Full reset to `UNPROVISIONED` |
| Cloud 409 (re-register) | Idempotent — `INSERT OR IGNORE` |
| Advertising timeout | Restart advertising cycle |

---

## Repository Structure

```
DeviceOS/
├── cloud/              # Go cloud platform (API, shadow, registry)
├── cli/                # Go CLI tool + simulator
│   └── simulator/      # State machine + BLE transport abstraction
├── dashboard/          # Next.js dashboard
├── runtime/            # C++ device runtime + HAL
├── sdk/                # C++ developer SDK
├── docs/               # Architecture Decision Records (ADRs)
└── tests/              # Integration tests
```

---

## Roadmap

- [x] v0.1.0-alpha — Simulation Platform MVP
- [ ] v0.2.0 — Flutter Companion App
- [ ] v0.3.0 — Enterprise Dashboard + Digital Twin
- [ ] v0.4.0 — Fleet Management + OTA Campaigns
- [ ] v1.0.0 — ESP32 Hardware Integration

---

## License

MIT © 2026 Karan Chhunchha
