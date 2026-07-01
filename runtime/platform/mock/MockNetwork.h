#ifndef DEVICEOS_MOCK_NETWORK_H
#define DEVICEOS_MOCK_NETWORK_H

#include "../../include/hal/INetwork.h"

namespace DeviceOS {
    namespace Platform {

        class MockNetwork : public HAL::INetwork {
        public:
            MockNetwork() noexcept;
            ~MockNetwork() override = default;

            // IDriverLifecycle
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_lifecycleState; }

            // INetwork
            Result<void> connect(const char* ssid, const char* passphrase) override;
            Result<void> disconnect() override;
            HAL::NetworkStatus getStatus() const noexcept override { return m_networkStatus; }
            Result<int16_t> getRSSI() const noexcept override;
            Result<HAL::IPAddress> getIPAddress() const noexcept override;

        private:
            LifecycleState m_lifecycleState;
            HAL::NetworkStatus m_networkStatus;
            char m_connectedSSID[32];
            HAL::IPAddress m_ipAddress;
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_NETWORK_H
