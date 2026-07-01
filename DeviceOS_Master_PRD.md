# DeviceOS Runtime — Master System Architecture & Product Requirements Document

## DOCUMENT CONTROL

* **Version:** 1.0.0-ArchSpec
* **Status:** Draft / Approved for Implementation
* **Author:** Principal Embedded Systems Architect & IoT Platform Engineer
* **Target Platforms:** ESP32, ESP32-S3, ESP32-C6, STM32, RP2040, Linux Gateway
* **Licensing Target:** Apache 2.0 / Enterprise Commercial Split

---

# 1. Executive Summary

DeviceOS Runtime is a highly decoupled, modular developer platform for intelligent, connected edge devices. Instead of standard custom IoT solutions that require developers to repeatedly write boilerplate code for networking, provisioning, state synchronization, security, and OTA, DeviceOS abstracts these complexities into a unified developer interface. 

DeviceOS standardizes the edge-to-cloud path through:
1. **DeviceOS SDK & Runtime:** An event-driven, hardware-abstracted runtime targeting ESP-IDF/FreeRTOS, bare-metal microcontrollers (STM32, RP2040), and Linux edge gateways.
2. **DeviceOS Cloud Platform:** A cloud-native orchestration service providing a scalable Device Registry, state synchronization engine (Device Shadow), and cryptographic Over-The-Air (OTA) rollout engine.
3. **Pluggable Architecture:** A driver and module abstraction layer allowing quick extensions for physical connectivity protocols (Matter, Thread, LoRa, BLE, cellular) and system events.

By adopting a strict event-driven system architecture, DeviceOS ensures offline-first reliability, high network throughput, and hardware-independent flexibility.

---

# 2. Product Vision

### The Problem
Traditional IoT development suffers from high repetitive-engineering costs. For each new product, developers write low-level code for Wi-Fi recovery, BLE GATT server provisioning, TLS 1.3 handshake maintenance, state serialization, flash storage caching, and raw OTA firmware streaming. This tight coupling between application logic and infrastructure logic results in fragile systems, delayed time-to-market, and security vulnerabilities.

### The DeviceOS Solution
DeviceOS abstracts networking, persistence, and state transitions behind a clean interface:

```cpp
#include <DeviceOS.h>

void setup() {
    Device.begin(); // Spawns system tasks, recovers state cache, initializes event bus
}

void loop() {
    // Non-blocking telemetry reporting; state-sync engine handles packaging and sending
    Device.state.set("temperature", 24.5); 
    Device.process(); // Serves internal event dispatching
}
```

### Key Performance Indicators (KPIs)
* **Out-of-Box Onboarding:** Bluetooth Low Energy (BLE) Wi-Fi provisioning completed in under 30 seconds.
* **Network Resilience:** Auto-reconnection state machine with exponential backoff and jitter; zero packet loss during brief outages.
* **Memory Overhead:** Total SRAM footprint under 120KB on target ESP32 microcontrollers.
* **Boot Overhead:** Boot time to event-loop execution under 100ms.
* **Update Success Rate:** >99.8% OTA completion rate through dual-partition rollback mechanisms.

---

# 3. Functional Requirements

| Requirement ID | Component | Title | Description | Priority |
| --- | --- | --- | --- | --- |
| FR-RUN-001 | Runtime | BLE Provisioning | Must support secure BLE GATT service configuration of Wi-Fi credentials and registration keys. | Critical |
| FR-RUN-002 | Runtime | WiFi Management | Must handle automatic Wi-Fi credential recovery, connection retries, and network transition logic. | Critical |
| FR-RUN-003 | Runtime | State Sync | Must synchronize local state properties with the Cloud Device Shadow. | Critical |
| FR-RUN-004 | Runtime | Offline Caching | Must save telemetry to non-volatile flash storage when disconnected, uploading it upon reconnect. | Critical |
| FR-RUN-005 | Runtime | Rule Engine | Must execute logic rules locally (e.g., GPIO trigger on sensor thresholds) when offline. | High |
| FR-RUN-006 | Runtime | Plugin System | Must allow dynamic initialization and registration of connectivity or sensor modules. | High |
| FR-CLD-001 | Cloud | Device Shadow | Must maintain a desired vs. reported state JSON/CBOR document for each device. | Critical |
| FR-CLD-002 | Cloud | OTA Deployment | Must support scheduled and targeted rolling firmware rollouts with failure detection thresholds. | Critical |
| FR-MOB-001 | Mobile | Provisioner | Must discover, pair, and transfer network credentials to devices over encrypted BLE. | Critical |
| FR-DSH-001 | Dashboard | Fleet Console | Must display live device statuses, telemetry streams, and logging data. | High |

---

# 4. Non-Functional Requirements

| Metric | Target | Verification Method |
| --- | --- | --- |
| **System Latency** | Local event loop dispatch latency: <50ms. Cloud-to-device round-trip latency: <200ms. | Telemetry timestamp validation; end-to-end loopback scripts. |
| **Throughput** | Ingestion gateway must support 100,000 active device connections scaling progressively. | K6 distributed load testing. |
| **Availability** | Cloud state sync and registry API availability: 99.95%. | Multi-region cluster deployment with active health monitoring. |
| **Resilience** | 100% recovery to stable partition if firmware crash loops occur. | Watchdog testing with faulty images. |
| **Flash Wear** | Flash wear-leveling allocation with write amplification factor (WAF) <1.5. | NVS partition endurance simulation. |

---

# 5. Architecture Overview

DeviceOS is designed around **Clean Architecture** principles. The system enforces strict boundary layers: the inner core handles pure system state and events, while outer layers handle device drivers, protocols, and cloud communication.

```
       ┌─────────────────────────────────────────────────────────┐
       │                 Application Code Layer                  │
       └────────────────────────────┬────────────────────────────┘
                                    │ SDK APIs
       ┌────────────────────────────▼────────────────────────────┐
       │                  DeviceOS SDK Core Core                 │
       │     (State Manager, Event Dispatcher, Plugin Manager)   │
       └────────────────────────────┬────────────────────────────┘
                                    │ Internal Event Bus
       ┌────────────────────────────▼────────────────────────────┐
       │              System Runtime Services Layer              │
       │    (BLE Onboarder, Sync Engine, NVS Cache, OTA Engine)   │
       └────────────────────────────┬────────────────────────────┘
                                    │ Hardware Abstraction (HAL)
       ┌────────────────────────────▼────────────────────────────┐
       │               Driver Abstraction Layer (DAL)            │
       │    (IGPIO, II2C, INetwork, ISecureElement, IFlash)      │
       └────────────────────────────┬────────────────────────────┘
                                    │ Target SDK APIs
       ┌────────────────────────────▼────────────────────────────┐
       │       Hardware Interfaces (ESP-IDF, STM32 HAL, etc.)    │
       └─────────────────────────────────────────────────────────┘
```

---

# 6. C4 Architecture

We leverage the C4 model (Context, Container, Component, Code) to document the software architecture of DeviceOS Runtime across its different boundaries.

---

# 7. Context Diagram

This diagram shows the high-level system boundary of DeviceOS Runtime, showing its interactions with users, mobile apps, the cloud console, and external telemetry systems.

```
+-----------------------------------------------------------------------------+
|                               DeviceOS System                               |
|                                                                             |
|  +--------------------+        Pairs via BLE        +--------------------+  |
|  |     Mobile App     |---------------------------->|  DeviceOS Runtime  |  |
|  |  (Flutter/iOS/And) |                             |   (Edge Firmware)  |  |
|  +---------+----------+                             +---------+----------+  |
|            |                                                  |             |
|            | REST / HTTPS API                                 | MQTT / TLS  |
|            |                                                  |             |
|            v                                                  v             |
|  +--------------------+                             +--------------------+  |
|  |   Web Dashboard    |                             |   DeviceOS Cloud   |  |
|  |  (Next.js Management)                            |  (Orchestration)   |  |
|  +---------+----------+                             +---------+----------+  |
|            |                                                  |             |
|            | REST / API                                       | HTTP Post   |
|            v                                                  v             |
|  +--------------------+                             +--------------------+  |
|  |    System User     |                             |  External Systems  |  |
|  | (Embedded Developer)                             |  (Datadog/Databricks) |
|  +--------------------+                             +--------------------+  |
+-----------------------------------------------------------------------------+
```

---

# 8. Container Diagram

The containers separating the device firmware, gateway brokers, and backend microservices:

```
+-----------------------------------------------------------------------------------+
| DeviceOS Runtime Containers                                                       |
|                                                                                   |
|  [Edge Device]                                                                    |
|  +-----------------------------------------------------------------------------+  |
|  |  +--------------------+     Internal Events      +--------------------+     |  |
|  |  |    DeviceOS SDK    |<========================>|   Drivers / HAL    |     |  |
|  |  +---------+----------+                          +--------------------+     |  |
|  |            |                                                                |  |
|  |            | Read/Write State Cache                                         |  |
|  |            v                                                                |  |
|  |  +--------------------+                                                     |  |
|  |  |   Flash Storage    |                                                     |  |
|  |  |  (NVS / LittleFS)  |                                                     |  |
|  |  +--------------------+                                                     |  |
|  +------------+----------------------------------------------------------------+  |
|               |                                                                   |
|               | MQTT/TLS (Port 8883)                                              |
|               v                                                                   |
|  [Cloud Infrastructure]                                                           |
|  +--------------------+      Publish Event          +--------------------+        |
|  |    MQTT Broker     |---------------------------->|   State Sync Svc   |        |
|  |    (EMQX Cluster)  |                             |       (Go REST)    |        |
|  +---------+----------+                             +---------+----------+        |
|            ^                                                  |                   |
|            | Publish/Subscribe                                | Write Shadow      |
|            v                                                  v                   |
|  +--------------------+      Read Meta & Auth       +--------------------+        |
|  |   Cloud Gateway    |---------------------------->|     PostgreSQL     |        |
|  |    (REST / WS)     |                             |  (Device Registry) |        |
|  +---------+----------+                             +--------------------+        |
|            ^                                                  ^                   |
|            | HTTPS / WSS API                                  | Query / Edit      |
|            v                                                  |                   |
|  +--------------------+                                       |                   |
|  |   Next.js Console  |---------------------------------------+                   |
|  +--------------------+                                                           |
+-----------------------------------------------------------------------------------+
```

---

# 9. Component Diagram

Detailing the internal component dependencies of the `DeviceOS Runtime` running on the edge:

```
+-------------------------------------------------------------------------------------+
| DeviceOS Runtime Component Layout                                                   |
|                                                                                     |
|  +-------------------------------------------------------------------------------+  |
|  |                               DeviceOS SDK Core                               |  |
|  |                                                                               |  |
|  |  +--------------------+      Pub/Sub Events       +--------------------+      |  |
|  |  |  Event Dispatcher  |<========================>|    State Manager   |      |  |
|  |  +---------+----------+                          +---------+----------+      |  |
|  |            |                                               |                 |  |
|  |            | Trigger Actions                               | Cache Deltas    |  |
|  |            v                                               v                 |  |
|  |  +--------------------+                          +--------------------+      |  |
|  |  |   Plugin Manager   |                          |    Storage Cache   |      |  |
|  |  +---------+----------+                          +---------+----------+      |  |
|  |            |                                               |                 |  |
|  +------------+-----------------------------------------------+-----------------+  |
|               |                                               |                    |
|               | Dynamic Loading Calls                         | Write Cache Blocks |
|               v                                               v                    |
|  +----------------------------+             +-----------------------------------+  |
|  |      Plugin Interface      |             |         Secure Storage            |  |
|  | (BLE / Matter / Lora / P2P) |             |       (NVS Wear-Leveling)         |  |
|  +----------------------------+             +-----------------------------------+  |
+-------------------------------------------------------------------------------------+
```

---

# 10. Runtime Architecture

The DeviceOS runtime leverages **FreeRTOS** under the hood. To ensure execution predictability, system tasks are partitioned with specific priorities and stack allocations:

```
+-------------------------------------------------------------------------------+
| FreeRTOS Tasks Priority Model                                                 |
|                                                                               |
| Priority 24 (Highest) -> [ Sys Watchdog & Monitoring Task (1KB Stack) ]       |
| Priority 20           -> [ Network Interface & TLS Link (8KB Stack) ]          |
| Priority 16           -> [ Event Dispatcher & Message Queue (4KB Stack) ]     |
| Priority 12           -> [ Local Rule Engine Execution (4KB Stack) ]          |
| Priority 8            -> [ Storage Controller & NVS Flush (3KB Stack) ]       |
| Priority 4 (Lowest)   -> [ Idle Hook & Hardware Stats Monitor (1KB Stack) ]    |
+-------------------------------------------------------------------------------+
```

### Core Event Loop State Transition Model

```
        +-----------------------------------------------------+
        |                     POWER_ON                        |
        +--------------------------+--------------------------+
                                   | Boot & Self-Test
                                   v
        +-----------------------------------------------------+
        |                    SYSTEM_INIT                      |
        +--------------------------+--------------------------+
                                   | Recover NVS / Read Configuration
                                   v
                    +--------------+--------------+
                    |                             |
      Provisioned == False          Provisioned == True
                    |                             |
                    v                             v
        +---------------------+         +---------------------+
        |   BLE_ONBOARDING    |         |    WIFI_CONNECTING  |
        +----------+----------+         +----------+----------+
                   | Successful Setup              | Connection Success
                   v                               v
        +-----------------------------------------------------+
        |                    MQTT_CONNECTING                  |
        +--------------------------+--------------------------+
                                   | TLS Handshake established
                                   v
        +-----------------------------------------------------+
        |                    STATE_SYNCED (Normal Operation)  |
        +--------------------------+--------------------------+
          | Network Lost           ^ Connection Restored      | Trigger OTA
          v                        |                          v
        +---------------------+    |            +---------------------+
        |    OFFLINE_MODE     |----+            |    OTA_UPDATING     |
        +---------------------+                 +---------------------+
```

---

# 11. SDK Design

The SDK defines a hardware-independent namespace in C++:

```cpp
namespace DeviceOS {

    class IEventSubscriber {
    public:
        virtual void onEvent(const char* topic, const uint8_t* payload, size_t length) = 0;
    };

    class EventBus {
    public:
        void publish(const char* eventTopic, const uint8_t* payload, size_t length);
        void subscribe(const char* eventTopic, IEventSubscriber* subscriber);
    };

    class StateManager {
    public:
        void set(const char* key, int32_t value);
        void set(const char* key, const char* value);
        void commit(); // Triggers event serialization and publishes to internal EventBus
        void onShadowSync(const uint8_t* patchPayload, size_t length);
    };

    class StorageEngine {
    public:
        bool writeSecure(const char* namespaceName, const char* key, const uint8_t* data, size_t length);
        bool readSecure(const char* namespaceName, const char* key, uint8_t* buffer, size_t* length);
    };

    class DeviceCore {
    private:
        EventBus eventBus;
        StateManager state;
        StorageEngine storage;
    public:
        void begin();
        void process(); // Serves watchdog and task queues
        
        EventBus& getEventBus() { return eventBus; }
        StateManager& getState() { return state; }
        StorageEngine& getStorage() { return storage; }
    };
}

extern DeviceOS::DeviceCore Device;
```

---

# 12. Driver Abstraction Layer

To ensure portability across hardware (ESP32, STM32, RP2040, Linux), the runtime interacts exclusively with virtual hardware interfaces (HAL/DAL):

```cpp
namespace DeviceOS {
    namespace Drivers {

        class IGPIO {
        public:
            virtual void setMode(uint8_t pin, uint8_t mode) = 0;
            virtual void write(uint8_t pin, uint8_t value) = 0;
            virtual uint8_t read(uint8_t pin) = 0;
        };

        class INetworkDriver {
        public:
            virtual bool connect(const char* ssid, const char* passphrase) = 0;
            virtual bool disconnect() = 0;
            virtual bool isConnected() = 0;
            virtual int16_t getRSSI() = 0;
        };

        class ISecureElement {
        public:
            virtual bool generateCSR(uint8_t* csrOut, size_t* csrLen) = 0;
            virtual bool signECDSA(const uint8_t* hash, size_t hashLen, uint8_t* signatureOut, size_t* sigLen) = 0;
        };
    }
}
```

---

# 13. BLE Specification

During `BLE_ONBOARDING`, the device operates as a BLE GATT Server advertising custom Service UUIDs:

* **Service UUID:** `4B50-DeviceOS-Provision-4F23`

### Characteristics

| Characteristic Name | UUID | Permissions | Payload Format | Description |
| --- | --- | --- | --- | --- |
| **SSID** | `4B50-0001` | WRITE | Plain Text UTF-8 | Target Wi-Fi SSID |
| **Passphrase** | `4B50-0002` | WRITE | Plain Text UTF-8 | Target Wi-Fi Password |
| **Auth Token** | `4B50-0003` | WRITE | Binary SHA-256 | Device registration secret token |
| **Status Channel** | `4B50-0004` | READ, NOTIFY | JSON / CBOR | State transition feedback |

### Provisioning Sequence Flow

```
Mobile App (Flutter)           Device (BLE Server)             Cloud API Gate
        |                               |                              |
        |--- Discover Device via BLE -->|                              |
        |<-- Returns BLE Advertising ---|                              |
        |                               |                              |
        |--- Encrypted TLS Handshake -->|                              |
        |                               |                              |
        |--- Write SSID/Passphrase ---->|                              |
        |    & Registration Token       |                              |
        |                               |--- Connects to Wi-Fi SSID -->|
        |                               |                              |
        |                               |--- Verify Register Token --->|
        |                               |<-- Return JWT Certificate ---|
        |<-- Notify Provision SUCCESS --|                              |
```

---

# 14. WiFi Specification

To prevent network loops and resource depletion, connection recovery runs on a strict exponential backoff algorithm:

$$t_{\text{backoff}} = \min(t_{\text{max}}, t_{\text{base}} \times 2^{\text{attempt}}) \pm \text{jitter}$$

* $t_{\text{base}} = 2 \text{ seconds}$
* $t_{\text{max}} = 300 \text{ seconds (5 minutes)}$
* $\text{jitter} = \text{Random Value between } -0.5 \text{ and } 0.5 \text{ seconds}$

```
                  +--------------------------------+
                  |         WIFI_DISCONNECTED      |
                  +---------------+----------------+
                                  |
                                  | Connection Request / Network Lost
                                  v
                  +--------------------------------+
                  |      INITIATE_CONNECTION       |
                  +---------------+----------------+
                                  |
                   Attempt Success? (Timeout = 15s)
                     /                          \
                    No                          Yes
                   /                              \
                  v                                v
    +------------------------------+     +--------------------+
    |      RETRY_BACKOFF           |     |   WIFI_CONNECTED   |
    | Calculate exponential delay  |     +--------------------+
    +--------------+---------------+
                   |
                   | Delay Expiration
                   v
    +------------------------------+
    |      RETRY_ATTEMPT           |
    +------------------------------+
```

---

# 15. MQTT Topic Design

MQTT communications leverage a strict, standardized namespace structures to enforce security and parsing optimizations.

```
devices/{tenant_id}/{device_id}/
├── telemetry/
│   └── {stream_name}    - (QoS 0, Raw Device Telemetry Data)
├── logs/stream           - (QoS 0, Structured Debug/Error Logs)
├── shadow/
│   ├── update            - (QoS 1, Device publishes current state / Cloud desired config)
│   ├── delta             - (QoS 1, Cloud pushes configuration changes to Device)
│   └── get               - (QoS 1, Request current cloud representation state)
└── rpc/
    ├── cmd/{cmd_name}    - (QoS 1, Cloud remote execution calls)
    └── response/{cmd_id} - (QoS 1, Device replies with completion status code)
```

---

# 16. State Synchronization Engine

State Synchronization acts as the Device Shadow bridge. The device and the cloud synchronize local and remote registries using patches encoded in **CBOR** (or fallback compact JSON).

### Structure of Shadow Payload

```json
{
  "version": 42,
  "reported": {
    "temperature": 24.5,
    "relay_state": true,
    "firmware_version": "v1.0.0"
  },
  "desired": {
    "relay_state": false,
    "target_temperature": 22.0
  }
}
```

```
Device (Reported)                     MQTT Broker (EMQX)                 Cloud Core (Desired)
      |                                        |                                    |
      |-- Pub: shadow/update (Reported Data) ->|                                    |
      |                                        |-- Forward Reported Data ---------->|
      |                                        |                                    |
      |                                        |                                    | (Evaluate state differences)
      |                                        |<-- Pub shadow/delta --------------|
      |<-- Sub shadow/delta -------------------|                                    |
      |                                        |                                    |
      | (Applies delta update Locally)         |                                    |
      |-- Pub shadow/update (Acknowledge) ---->|                                    |
      |                                        |-- Forward Acknowledge ------------>|
```

---

# 17. Offline Cache

When the network is unreachable, DeviceOS switches to **Offline-First Mode**. Telemetry packets are written to a circular buffer partition on the local SPI Flash.

```
                  +--------------------------------+
                  |         Telemetry Event        |
                  +---------------+----------------+
                                  |
                           Network Available?
                             /            \
                           Yes            No
                           /                \
                          v                  v
                  +---------------+  +-------------------------------+
                  |  Publish Raw  |  |      SPIFFS/LittleFS Queue    |
                  |     MQTT      |  | Write with priority metadata |
                  +---------------+  +---------------+---------------+
                                                     |
                                            Flash Cache Limit Exceeded?
                                              /                     \
                                            Yes                     No
                                            /                         \
                                           v                           v
                      +-----------------------------+         +-------------------+
                      |      Prune Low-Priority     |         | Commit Block to   |
                      |      Telemetry Records      |         | circular memory   |
                      +-----------------------------+         +-------------------+
```

---

# 18. Conflict Resolution

When a device connects after an extended offline period, state differences between the device and cloud are resolved using:
1. **Monotonically Increasing Version Counters:** Evaluated by the Cloud Shadow Engine.
2. **Server-Authoritative Last-Write-Wins (LWW):** For device-side physical measurements (like temperature), the device's timestamp takes precedence. For configuration parameters (like relays or operational modes), the cloud's timestamp takes precedence.

### Resolution Protocol Matrix

| Variable Type | Conflict Scenario | Precedence | Action |
| --- | --- | --- | --- |
| Physical Telemetry | Cloud timestamp = 12:00, Device offline buffer timestamp = 12:05 | Device State | Overwrite cloud shadow with device's timestamped state history. |
| Operational Config | Cloud changes relay setting to OFF, Device starts up reporting state ON | Cloud State | Force local Device driver to adjust GPIO pin according to cloud state shadow. |

---

# 19. OTA Architecture

Over-the-Air updates use HTTPS or chunked MQTT streams to deploy firmware:

```
[System Partition Layout]
+-------------------------------------------------------------------------------------------------+
| Bootloader | Partition Table | Factory Recovery |   active_ota_0   |   active_ota_1   | NVS Data |
|   (32KB)   |      (3KB)      |      (1MB)       |     (1.5MB)      |     (1.5MB)      |  (512KB) |
+-------------------------------------------------------------------------------------------------+
```

### Chunk Verification Pipeline
1. Device downloads file block $N$.
2. Write block to target inactive OTA partition in flash.
3. Compute rolling SHA-256 hash.
4. On last block download, verify final computed SHA-256 against signature signed with ECDSA-256.

---

# 20. Bootloader

The bootloader configures boot flags in the secure OTP memory or RTC registers:

```
                  +--------------------------------+
                  |            Boot Reset          |
                  +---------------+----------------+
                                  |
                                  v
                  +--------------------------------+
                  |       Verify System Check      |
                  +---------------+----------------+
                                  |
                            Self-Test OK?
                             /          \
                           Yes           No (or Crash Loop Triggered)
                           /              \
                          v                v
            +--------------------------+  +--------------------------------+
            |  Load Inactive OTA App   |  |   Revert to Stable Factory /   |
            |     Update boot flags    |  |   Alternative boot partition   |
            +--------------------------+  +--------------------------------+
```

---

# 21. Security

DeviceOS Runtime operates under a strict Zero-Trust Model:
1. **Network Protection:** TLS 1.3 encryption on all external network links (HTTPS, WSS, MQTTS).
2. **Hardware Integrity:** Support for hardware cryptographic modules (ATECC608, ESP32 eFuse).
3. **Firmware Decoupling:** Hardware-enforced compartmentalization.

---

# 22. Authentication

Devices authenticate using **Mutual TLS (mTLS)**:

```
Device (Unique Keypair)                                                   Cloud Auth CA
      |                                                                          |
      |-- Certificate Signing Request (CSR) with device HW details ------------->|
      |                                                                          | (Validate registration signature)
      |<-- Signed Device Certificate (X.509) returned ---------------------------|
      |                                                                          |
      |-- Establish MQTTS connection using X.509 Client Cert ------------------->|
      |<-- Access Granted to exclusive topics namespace -------------------------|
```

---

# 23. Device Identity

Device identity is constructed from static hardware attributes to prevent spoofing:

$$\text{DeviceUID} = \text{SHA256}(\text{MAC Address} \parallel \text{eFuse Cryptographic Root Key})$$

The DeviceUID acts as the unique identifier for device routing, logging, and key signature derivation.

---

# 24. Certificate Management

Certificates have an active lifetime of 365 days. At day 300, the background runtime schedules an automated renewal operation:

```
                  +--------------------------------+
                  |    Periodic Certificate Check  |
                  +---------------+----------------+
                                  |
                            Days Left <= 65?
                             /            \
                            No            Yes
                           /                \
                          v                  v
                  +---------------+  +-------------------------------+
                  |  Idle Return  |  |   Generate CSR on secure chip |
                  +---------------+  +---------------+---------------+
                                                     |
                                                     v
                                     +-------------------------------+
                                     |   Send CSR via shadow topic   |
                                     +---------------+---------------+
                                                     |
                                                     v
                                     +-------------------------------+
                                     | Replace cert in memory & NVS  |
                                     +-------------------------------+
```

---

# 25. Flash Encryption

Hardware Flash Encryption prevents physical data extraction:
* ESP32 runtime configures AES-256 internal key burned directly into eFuse blocks.
* Encryption keys are locked using hardware control lines, making them inaccessible to software.
* Code execution paths decrypt blocks on-the-fly during cache fetches from SPI flash.

---

# 26. Secure Boot

Secure Boot V2 ensures only validated images execute:

```
[Hardware Secure Boot Pipeline]
+-----------------------------------+
| eFuse Secure Boot Keys Hash       |
+-----------------+-----------------+
                  | Compare Signature Hash
                  v
+-----------------+-----------------+
| First Stage Bootloader Signature  |
+-----------------+-----------------+
                  | Verify Bootloader
                  v
+-----------------+-----------------+
| SDK Partition Signature           |
+-----------------+-----------------+
                  | Verify App Image
                  v
+-----------------+-----------------+
| Executable Runtime Environment    |
+-----------------------------------+
```

---

# 27. Event Bus

The runtime Event Bus decouples different software components. Components publish system event packets without knowing who subscribes to them.

### Event Definition Struct

```cpp
struct SystemEvent {
    uint32_t timestamp;
    uint16_t event_id;
    uint8_t priority;
    size_t payload_len;
    uint8_t payload[256];
};
```

### System Event IDs

```
0x0001: SYS_BOOT_COMPLETE
0x0002: NET_WIFI_CONNECTED
0x0003: NET_WIFI_DISCONNECTED
0x0004: MQTT_CONNECTED
0x0005: STATE_SHADOW_DELTA_RECEIVED
0x0006: DEVICE_OTA_START
0x0007: DRIVER_PIN_INTERRUPT
```

---

# 28. Plugin System

Plugins are isolated modules that register directly to the runtime's dynamic registry. They can register callbacks to hook into the System Event Bus.

```cpp
class IPlugin {
public:
    virtual const char* getPluginName() = 0;
    virtual void onInit() = 0;
    virtual void onStart() = 0;
    virtual void onStop() = 0;
};

class PluginRegistry {
private:
    std::vector<IPlugin*> activePlugins;
public:
    void registerPlugin(IPlugin* plugin) {
        activePlugins.push_back(plugin);
        plugin->onInit();
    }
    
    void startAll() {
        for(auto& plugin : activePlugins) {
            plugin->onStart();
        }
    }
};
```

### Dynamic Plugin Registration

```
                           +------------------------+
                           |     Load Plugin Bin    |
                           +-----------+------------+
                                       |
                                       v
                           +------------------------+
                           |  Check Entry Callbacks |
                           +-----------+------------+
                                       |
                                       v
                           +------------------------+
                           |   Call Plugin->onInit  |
                           +-----------+------------+
                                       |
                                       v
                           +------------------------+
                           | Register Event Hooks   |
                           | to Runtime System Bus  |
                           +------------------------+
```

---

# 29. Rule Engine

The local rule engine parses simple conditional logic expressions written in Abstract Syntax Trees (AST) or static JSON arrays:

```json
{
  "rule_id": "temp_safety",
  "trigger": "state.temperature",
  "condition": ">",
  "value": 45.0,
  "action": {
    "type": "gpio_write",
    "pin": 12,
    "value": 1
  }
}
```

This local evaluation model guarantees safety operations continue executing even during network dropouts.

---

# 30. Automation Engine

The cloud automations engine lets developers define complex cross-device rule workflows. These rule workflows are compiled into execution payloads and distributed to devices via State Shadow channels.

```
Cloud Console Rule Definition (UI)
        |
        |--- User saves multi-device action rule --->
        |
        v
    Rule Compiler Engine (Cloud Services)
        |
        |--- Compiles logic rules to static JSON binary ---
        |
        v
    State Sync (Shadow Delta Publish)
        |
        |--- Push compiled rules update to local devices ---
        |
        v
    Device Runtime (Rule Engine)
        |
        |--- Loads rules into memory, updates event hooks ---
```

---

# 31. Digital Twin

The Digital Twin maintains an active logical mapping of each device's state in the PostgreSQL database.

### Digital Twin Model Schema

```json
{
  "device_id": "dev_c6_01fa92c",
  "tenant_id": "tenant_default",
  "connection_status": "ONLINE",
  "last_communication": "2026-07-01T15:20:00Z",
  "system_stats": {
    "free_heap_bytes": 142096,
    "cpu_temp_c": 38.5,
    "wifi_rssi_dbm": -67
  },
  "configurations": {
    "logging_level": "DEBUG",
    "telemetry_interval_sec": 30
  }
}
```

---

# 32. AI Integration

The Cloud platform integrates an AI agent pipeline to analyze fleet data:
1. **Device Anomaly Diagnostics:** Predicts hardware degradation (e.g., flash memory bad blocks or power supply drops) using telemetry trend analysis.
2. **Predictive Failure Warnings:** Uses anomalous temperature deviations to alert administrators about impending hardware issues.
3. **Natural Language System Rules:** Translates human requirements (e.g., *"If temperature goes over 40 for more than 5 minutes, turn on the alarm relay"* ) into rule engine payloads.

---

# 33. REST API

The cloud service exposes a REST API for management dashboards:

### Endpoints

`POST /v1/devices/register`
* Register a new device token.
* **Payload:** `{ "device_uid": "...", "tenant_id": "..." }`
* **Response:** `{ "client_cert_pem": "...", "private_key_pem": "..." }` (Key generated cloud-side or CSR-backed)

`GET /v1/devices/{device_id}/shadow`
* Fetch the current desired and reported state shadow of the device.

`POST /v1/ota/deployments`
* Trigger a new firmware deployment rollout.
* **Payload:** `{ "firmware_version": "v1.0.2", "target_tags": { "building": "A" } }`

---

# 34. WebSocket API

For low-latency console updates, the web interface establishes connection via WebSockets:

* **Endpoint:** `wss://api.deviceos.platform/v1/console`

### Message Types

* `log_subscribe`: Subscribes to live device logging streams.
* `log_payload`: Contains debug, info, and error outputs from the device runtime.
* `telemetry_live`: Publishes live device state data on value transitions.

---

# 35. Database Design

The relational registry and dynamic telemetry metrics storage run on a optimized PostgreSQL instance:

```sql
-- Devices Registry Table
CREATE TABLE devices (
    device_id VARCHAR(64) PRIMARY KEY,
    tenant_id VARCHAR(64) NOT NULL,
    secret_token VARCHAR(128) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    last_boot_at TIMESTAMP WITH TIME ZONE
);

-- Device State Shadows Table
CREATE TABLE device_shadows (
    device_id VARCHAR(64) REFERENCES devices(device_id) ON DELETE CASCADE,
    version INT NOT NULL DEFAULT 1,
    reported JSONB NOT NULL DEFAULT '{}',
    desired JSONB NOT NULL DEFAULT '{}',
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (device_id)
);

-- OTA Images Metadata Table
CREATE TABLE firmware_images (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    version VARCHAR(32) NOT NULL,
    hardware_target VARCHAR(64) NOT NULL,
    sha256_hash VARCHAR(64) NOT NULL,
    file_path VARCHAR(256) NOT NULL,
    uploaded_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
```

---

# 36. Repository Structure

We enforce a professional, unified monorepo repository structure:

```
deviceos/
├── runtime/                 # Device C++ runtime core code
│   ├── src/                 # System task logic and event loops
│   ├── include/             # Interface declarations
│   └── platform/            # Porting interfaces (esp32, stm32, linux)
├── sdk/                     # Developer API bindings
│   ├── cpp/                 # C++ SDK files
│   └── python/              # Python helper tools
├── gateway/                 # Edge gateway runtime binaries
├── cloud/                   # Go-based cloud services API
│   ├── registry/            # Device management module
│   ├── sync/                # State synchronization shadow processor
│   └── ota/                 # OTA binary upload processor
├── dashboard/               # Next.js web application console
├── mobile/                  # Flutter application
├── cli/                     # Developer management CLI code
├── tests/                   # Unified verification test suites
│   ├── integration/         # Cloud APIs and WebSockets E2E tests
│   └── hil/                 # Hardware-in-the-Loop test setups
└── infrastructure/          # Deployment configurations
    ├── docker/              # Dockerfiles for dev services
    └── kubernetes/          # K8s manifest files and Helm charts
```

---

# 37. CLI

The Developer Experience (DX) CLI is the primary interface for managing devices locally and in the cloud:

```bash
# Initialize a new DeviceOS workspace directory
deviceos init --target esp32

# Flash compiled binary and configurations to a local device
deviceos flash --port COM3 --config ./device_config.json

# Stream live debugging and trace logs directly from the device
deviceos logs --device-id dev_c6_01fa92c

# Remote trigger an OTA rollout to a target device group
deviceos ota deploy --version v1.0.2 --target-group "staging"

# Test local hardware interfaces and connection status
deviceos doctor
```

---

# 38. DevOps

All cloud systems are containerized. High-throughput telemetry and REST components deploy on Kubernetes clusters:

```yaml
# Sample Kubernetes Telemetry Ingestion Deployment Manifest
apiVersion: apps/v1
kind: Deployment
metadata:
  name: deviceos-telemetry-processor
  namespace: deviceos-core
spec:
  replicas: 3
  selector:
    matchLabels:
      app: telemetry-processor
  template:
    metadata:
      labels:
        app: telemetry-processor
    spec:
      containers:
      - name: processor
        image: deviceos/telemetry-processor:v1.0.1
        ports:
        - containerPort: 8080
        resources:
          limits:
            cpu: "2"
            memory: 2Gi
          requests:
            cpu: "500m"
            memory: 512Mi
```

---

# 39. CI/CD

Automated CI/CD pipelines compile firmware targets and run tests on every pull request:

```
                  +--------------------------------+
                  |         Git Pull Request       |
                  +---------------+----------------+
                                  |
                                  v
                  +--------------------------------+
                  |     Linter & Syntax Check      |
                  +---------------+----------------+
                                  |
                                  v
                  +--------------------------------+
                  |     Compile Firmware Targets   |
                  |  (using ESP-IDF CLI Docker)    |
                  +---------------+----------------+
                                  |
                                  v
                  +--------------------------------+
                  |       Run Unit Tests Core      |
                  +---------------+----------------+
                                  |
                                  v
                  +--------------------------------+
                  |    Deploy Artifacts / Images   |
                  |     to staging registries      |
                  +--------------------------------+
```

---

# 40. Testing Strategy

The test plan is divided into three key layers:
1. **Unit Tests:** C++ mocks testing local queues, storage partitions, and state transitions. Go unit tests testing registry REST APIs.
2. **Integration Tests:** Automated scripts verifying MQTT payload serialization, socket connections, and shadow delta patches.
3. **Load Tests:** Simulating traffic spikes using load-generation engines to ensure system scalability.

---

# 41. Hardware in the Loop (HIL)

HIL testing automates firmware verification on physical target hardware:

```
+-----------------------------------------------------------------------------------+
| HIL Testing Pipeline Setup                                                        |
|                                                                                   |
|  +--------------------+      Flashes Image         +--------------------+         |
|  |    Test Runner     |===========================>|    Physical ESP32  |         |
|  | (Github Agent PC)  |                            |   Development Board|         |
|  +---------+----------+                            +---------+----------+         |
|            ^                                                 |                    |
|            |                                                 | Toggles IO lines   |
|            | Validates Console outputs                       v                    |
|            |                                       +--------------------+         |
|            +---------------------------------------|    GPIO Analyzer   |         |
|                                                    |   & Power Switch   |         |
|                                                    +--------------------+         |
+-----------------------------------------------------------------------------------+
```

---

# 42. Deployment

Firmware rollouts are deployed progressively to mitigate system outages:

```
Stage 1: Internal Canary (1% of Fleet)
   |
   | (Monitored for 24 hours for crashes/reconnects)
   v
Stage 2: Staging Beta (10% of Fleet)
   |
   | (Monitored for 48 hours for data errors)
   v
Stage 3: General Rollout (100% of Fleet)
```

Rollouts are automatically suspended if target device crash logs or connectivity dropouts exceed a 1% error threshold.

---

# 43. Roadmap

```
Phase 1: Foundation (Months 1-3)
* Complete Driver Abstraction Layer interfaces (IGPIO, INetwork).
* Core BLE provisioning GATT service implementation.
* Local circular offline cache with wear-leveling.

Phase 2: Cloud Sync (Months 4-6)
* Launch Go-based Registry and Device Shadow APIs.
* Implement state synchronization delta loops.
* Release developer CLI commands (init, flash).

Phase 3: Rollouts & Management (Months 7-9)
* Dual-partition secure boot OTA updater on ESP32 runtime.
* Next.js Console interface showing live states and logs.
* Advanced rules engine evaluation locally.

Phase 4: Swarm & Matter (Months 10-12)
* Matter/Thread plugin modules integration.
* P2P mesh network communication testing.
* Edge failure warnings via telemetry analysis.
```

---

# 44. Open Source Strategy

### License
* **Core Runtime & SDK:** Apache 2.0 License (encourages developer contributions and integration).
* **Enterprise Modules:** Commercial license model covering advanced features (multi-region cloud clustering, advanced OTA rollout groups, custom SSO integrations).

### Community Engagement
To encourage developer adoption, DeviceOS will maintain a public Registry Portal where developers can share community plugins (e.g., custom sensor drivers, custom local rules) and contribute core feature enhancements.
