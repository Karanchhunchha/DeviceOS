# DeviceOS Companion App - User Guide

Welcome to the DeviceOS Companion App! This app allows you to provision your IoT devices, connect them to your local Wi-Fi, and monitor their live telemetry data via the Cloud.

## Getting Started

### 1. Installation
Currently, the app can be installed via APK on Android devices.
- Download the latest `deviceos_companion.apk`.
- Open the file on your Android device and tap **Install**.

### 2. First-time Setup & Permissions
When you open the app for the first time, you will see the **Onboarding Screen**.
The app requires the following permissions to function correctly:
- **Bluetooth**: To scan for and connect to real IoT devices over BLE.
- **Location**: Required by Android to perform Bluetooth scanning.
- **Notifications**: To alert you of any device state changes (if supported).

Tap **Grant Permissions** and allow them in the system dialog. If you deny them, the app will fall back to **Simulator Mode**, which doesn't require these permissions.

---

## Provisioning a Device

You can provision a real device or a simulated device.

### Simulator Mode (Default)
If you don't have real hardware, you can test the platform using Simulator Mode.
1. Select **Simulator Mode (HTTP)** on the Provisioning card.
2. Enter the **Device Local Port** (e.g., `9001`).
3. Enter your **Wi-Fi SSID** and **Password**.
4. Enter your **Tenant ID** (default: `tenant_default`).
5. Tap **Provision Wireless Network**.

### Real BLE Mode
If you have a real ESP32 device running DeviceOS:
1. Select **Real BLE Mode**.
2. Enter the **Device BLE Address / MAC** (e.g., `4B:50:AA:BB:CC:DD`).
3. Provide your Wi-Fi details.
4. Tap **Provision Wireless Network**.

The app will connect to the device via Bluetooth and securely send the Wi-Fi credentials. Once successful, the device will connect to the cloud.

---

## Live Telemetry & Fleet Monitoring

Once your device is provisioned, you can monitor its data.

1. Scroll down to the **Device Shadow Synchronization** card.
2. Ensure the **Cloud Server Gateway** is correct (e.g., `http://your-server-ip:8082`).
3. Select your device from the **Fleet Dropdown**. (If it just provisioned in Simulator mode, it may take a few seconds to appear).
4. Tap **Sync**.

The **Live Digital Twin** card will appear, showing:
- Real-time **Temperature** and **Humidity** with historical graphs.
- Device **Network Status** (ONLINE/OFFLINE).
- You can toggle the **Relay State (LED Switch)** to send desired configurations back to the device.

---

## Frequently Asked Questions (FAQ)

**Q: What does "BLE timeout" mean?**
A: This usually means the app couldn't find the device via Bluetooth. Make sure your device is powered on, in range, and currently advertising its BLE service. 

**Q: Can I use the app without location permission?**
A: Android requires Location permission to scan for Bluetooth devices. If you don't grant it, you can only use the Simulator Mode.

**Q: My device says OFFLINE in the Live Digital Twin.**
A: Check if the device has lost Wi-Fi connection, or if your Cloud Server Gateway address is incorrect.

---
*For more technical details, refer to the developer documentation.*
