#ifndef DEVICEOS_MOCK_POWER_H
#define DEVICEOS_MOCK_POWER_H

#include "../../include/hal/IPower.h"

namespace DeviceOS {
    namespace Platform {

        class MockPower : public HAL::IPower {
        public:
            MockPower() noexcept;
            ~MockPower() override = default;

            // IDriverLifecycle
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_lifecycleState; }

            // IPower
            Result<void> enterLightSleep(uint64_t durationMs) override;
            Result<void> enterDeepSleep(uint64_t durationMs) override;
            HAL::WakeReason getWakeReason() const noexcept override { return m_wakeReason; }
            Result<void> softwareReset() override;
            Result<float> getBatteryVoltage() override;
            Result<uint8_t> getBatteryPercentage() override;

            // Direct simulation control
            void setWakeReason(HAL::WakeReason reason) noexcept { m_wakeReason = reason; }

        private:
            LifecycleState m_lifecycleState;
            HAL::WakeReason m_wakeReason;
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_POWER_H
