import 'package:flutter_ble_peripheral/flutter_ble_peripheral.dart';
import 'package:flutter/foundation.dart';

class BlePeripheralManager {
  final FlutterBlePeripheral _blePeripheral = FlutterBlePeripheral();
  bool _isAdvertising = false;

  bool get isAdvertising => _isAdvertising;

  Future<void> startAdvertising() async {
    if (await _blePeripheral.isSupported) {
      final AdvertiseData advertiseData = AdvertiseData(
        serviceUuid: '4b500000-0000-0000-0000-000000004f23', // Matches ble_gatt_provisioner
        includeDeviceName: true,
      );

      final AdvertiseSettings advertiseSettings = AdvertiseSettings(
        advertiseMode: AdvertiseMode.advertiseModeBalanced,
        txPowerLevel: AdvertiseTxPower.advertiseTxPowerMedium,
        connectable: true,
      );

      await _blePeripheral.start(
        advertiseData: advertiseData,
        advertiseSettings: advertiseSettings,
      );

      _isAdvertising = true;
      debugPrint("Started BLE Advertisement (Host Peer Mode)");
    } else {
      debugPrint("BLE Peripheral mode is not supported on this device.");
      throw Exception("BLE Peripheral mode is not supported on this device.");
    }
  }

  Future<void> stopAdvertising() async {
    await _blePeripheral.stop();
    _isAdvertising = false;
    debugPrint("Stopped BLE Advertisement");
  }
}
