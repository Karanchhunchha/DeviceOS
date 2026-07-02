abstract class ProvisioningTransport {
  /// Provision a target device with Wi-Fi credentials.
  /// [target] represents:
  /// - An IP/Port string (e.g., "127.0.0.1:9001") for the simulated HTTP transport.
  /// - A Bluetooth Device MAC/UUID/Name for the real BLE transport.
  Future<void> provision({
    required String target,
    required String ssid,
    required String password,
    required String tenantId,
  });
}
