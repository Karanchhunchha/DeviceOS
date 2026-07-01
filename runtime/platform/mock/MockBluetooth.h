#ifndef DEVICEOS_MOCK_BLUETOOTH_H
#define DEVICEOS_MOCK_BLUETOOTH_H

#include "../../include/hal/IBluetooth.h"

namespace DeviceOS {
    namespace Platform {

        class MockBluetooth : public HAL::IBluetooth {
        public:
            MockBluetooth() noexcept;
            ~MockBluetooth() override = default;

            // IDriverLifecycle
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_lifecycleState; }

            // IBluetooth
            Result<void> registerGATTService(const HAL::GATTService& service) override;
            Result<void> startAdvertising(const char* deviceName) override;
            Result<void> stopAdvertising() override;
            Result<void> notifyCharacteristic(const char* charUuid, const uint8_t* data, size_t len) override;
            HAL::BLEState getBLEState() const noexcept override { return m_bleState; }

        private:
            LifecycleState m_lifecycleState;
            HAL::BLEState m_bleState;
            char m_advertisedName[32];
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_BLUETOOTH_H
