<div align="center">

# DeviceOS

**An Advanced IoT Runtime, Cloud Platform, Simulator, and Companion App**  
*Build, test, and manage connected devices — with zero hardware dependencies and seamless end-to-end integration.*

[![Version](https://img.shields.io/badge/version-v0.2.0--beta-blue?style=flat-square)](https://github.com/Karanchhunchha/DeviceOS/releases)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)
[![Go](https://img.shields.io/badge/Go-1.22-00ADD8?style=flat-square&logo=go)](https://golang.org)
[![Flutter](https://img.shields.io/badge/Flutter-3.x-02569B?style=flat-square&logo=flutter)](https://flutter.dev)
[![Status](https://img.shields.io/badge/status-active--success-brightgreen?style=flat-square)]()

---

<p align="center">
  📘 <a href="DeviceOS_Master_PRD.md">Master PRD</a> &nbsp;•&nbsp;
  🏗 <a href="docs/adr/005_bidirectional_state_synchronization.md">Architecture</a> &nbsp;•&nbsp;
  📍 <a href="#roadmap">Roadmap</a>
</p>

</div>

---

> **🚀 Project Status: End-to-End Success! (Beta)**  
> The core infrastructure of DeviceOS is officially ALIVE! We have successfully engineered and verified the complete architecture: **BLE Provisioning → Go Cloud Backend → WebSocket Streaming → Live Flutter Digital Twin.** 

## Project Achievements (v0.2.0)

**Overall Completion: ~60% (Foundation & Cloud Sync completely verified)**

### 🔥 Highlighted Features
1. **Real BLE Provisioning (App-to-App Mock):** The Flutter Companion app can now successfully act as a BLE Advertiser (simulating an ESP32), while a second phone scans, connects, and provisions Wi-Fi credentials over a GATT connection. A massive milestone for hardware-free development!
2. **Live Digital Twin (WebSockets):** The Flutter App features a real-time Live Digital Twin dashboard. Temperature and Humidity metrics are streamed via WebSockets from the Cloud server, painting beautiful, dynamic sparkline charts instantly.
3. **Robust Go Cloud Gateway:** A highly resilient Go backend that handles device registration, Device Shadow state synchronization, and WebSocket broadcasting with zero lag.
4. **Python Hardware Simulator:** A lightweight edge simulator (`simulate_esp32.py`) that successfully mimics an ESP32 connecting to Wi-Fi, generating telemetry, and pushing shadow updates to the Cloud.

---

## What is DeviceOS?

DeviceOS is a production-grade, hardware-independent IoT platform. It allows developers to **design, simulate, provision, monitor, and manage connected devices** before purchasing any physical hardware.

The platform is built around a core architectural principle:

> **Hardware is just another transport implementation.**

Everything from the provisioning flow, device shadow, telemetry pipeline, and OTA engine runs identically whether backed by a software simulator today or an ESP32 tomorrow.

---

## Architecture Flow

```
┌─────────────────────────────────────────────────┐
│              Flutter Companion App              │
│   (BLE Scanner & Live Digital Twin Dashboard)   │
└──────────────────────┬──────────────────────────┘
                       │ 
                       │ 1. Secure BLE GATT Provisioning (SSID/Password)
                       ▼
┌─────────────────────────────────────────────────┐
│     Edge Hardware / Simulator (Python/C++)      │
│   (Receives Credentials -> Connects to Wi-Fi)   │
└──────────────────────┬──────────────────────────┘
                       │ 
                       │ 2. Telemetry Push & State Sync (HTTP/MQTT)
                       ▼
┌─────────────────────────────────────────────────┐
│              Go Cloud Platform                  │
│  Device Registry · Shadow API · WebSocket Hub   │
└──────────────────────┬──────────────────────────┘
                       │
                       │ 3. WebSocket Real-Time Stream
                       ▼
┌─────────────────────────────────────────────────┐
│              Flutter Companion App              │
│       (Renders Live Sparklines & Metrics)       │
└─────────────────────────────────────────────────┘
```

---

## Components

| Component | Technology | Status |
|-----------|-----------|--------|
| **Flutter Companion App** | Flutter 3, Dart | ✅ **Complete (Tested on real Android devices via BLE & LAN)** |
| **Cloud Platform** | Go, SQLite, WebSockets | ✅ **Complete (Resilient & Fast)** |
| **Device Simulator** | Python / Go | ✅ **Complete (Live Telemetry Working)** |
| C++ Runtime | C++17, CMake | ✅ Complete |
| Hardware Abstraction Layer | C++ Interfaces | ✅ Complete |
| Provisioning Flow (E2E) | BLE → Wi-Fi | ✅ Complete |
| Device Shadow API | REST | ✅ Complete |
| OTA Engine (Core) | C++ | ⚠️ Mocks Complete |
| Dashboard | Next.js 15, React | ✅ Complete |

---

## Quick Start (Run it Yourself!)

### 1. Start the Go Cloud Server
```bash
cd cloud
go run ./cmd/server
# → Starts Registry and WebSocket Gateway on 0.0.0.0:8082
```

### 2. Start the ESP32 Simulator
```bash
python simulate_esp32.py
# → Begins pushing fake Temperature & Humidity data to the Go Server
```

### 3. Run the Flutter Companion App
```bash
cd mobile
flutter run
# → Open "Device Shadow Synchronization"
# → Enter your Laptop's Local IP (e.g., 10.101.104.32:8082)
# → Hit "Sync" and watch the Live Digital Twin graphs dance!
```

---

## Roadmap

- [x] **v0.1.0-alpha** — Simulation Platform MVP
- [x] **v0.2.0-beta** — **Flutter Companion App (Real BLE Mocking, Live Digital Twin, WebSocket Sync)** 🎉
- [ ] v0.3.0 — Offline Rules Engine (Local Logic Execution)
- [ ] v0.4.0 — Fleet Management + OTA Deployment Engine
- [ ] v1.0.0 — ESP32 Real Hardware Integration

---

## License

MIT © 2026 Karan Chhunchha
