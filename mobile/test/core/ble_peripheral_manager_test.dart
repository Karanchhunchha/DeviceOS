import 'package:flutter_test/flutter_test.dart';
import 'package:deviceos_companion/core/ble_peripheral_manager.dart';

void main() {
  group('BlePeripheralManager Tests', () {
    test('Initial state is not advertising', () {
      final manager = BlePeripheralManager();
      expect(manager.isAdvertising, isFalse);
    });

    // Note: To test startAdvertising(), we would need to mock FlutterBlePeripheral using Mockito
    // and the MethodChannel. Since this requires platform-specific mock channels, we focus on state.
  });
}
