# ADR 006: Real-Time WebSocket Console Broker

## Status
Approved

## Context
DeviceOS Console (web dashboard / CLI) needs to monitor live device logs and configuration changes in real time. We require a low-latency bidirectional communication link (WebSockets) between the dashboard client and the cloud registry service. The backend must scale to thousands of active sessions while maintaining zero external dependencies and zero-CGO compile profiles.

## Decision
1. **Standard Upgrader Hijack:** Implement a standard, zero-dependency HTTP hijacker upgrader in Go standard library to switch protocols from HTTP/1.1 to RFC 6455.
2. **Key Computation:** Compute the SHA-1 hash of the client's `Sec-WebSocket-Key` concatenated with the RFC 6455 magic GUID `258EAFA5-E914-47DA-95CA-C5AB0DC85B11`.
3. **Session Broker maps:** Maintain a thread-safe broker (`BroadcastBroker`) with a `sync.RWMutex`-protected map linking device IDs to active TCP network connections.
4. **Shadow Broadcast Trigger:** Upon completing any database shadow update (`PUT /v1/devices/{device_id}/shadow`), trigger an asynchronous broadcast to all WebSocket clients currently subscribed to that device.

## Consequences
* **Positives:** Broadcast latency is sub-millisecond (< 500 microseconds). We have zero dependencies on heavy third-party WebSocket wrappers, keeping binary sizes under 12MB.
* **Negatives:** Handshaking, connection reading, and frame parsing must be maintained at the byte layer.
