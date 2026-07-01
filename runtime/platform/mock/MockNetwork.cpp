#include "MockNetwork.h"
#include <string.h>

namespace DeviceOS {
    namespace Platform {

        MockNetwork::MockNetwork() noexcept
            : m_lifecycleState(LifecycleState::UNINITIALIZED),
              m_networkStatus(HAL::NetworkStatus::DISCONNECTED)
        {
            m_connectedSSID[0] = '\0';
            m_ipAddress.version = HAL::IPVersion::IPv4;
            m_ipAddress.address.ipv4[0] = 0;
            m_ipAddress.address.ipv4[1] = 0;
            m_ipAddress.address.ipv4[2] = 0;
            m_ipAddress.address.ipv4[3] = 0;
        }

        Result<void> MockNetwork::initialize() {
            m_lifecycleState = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockNetwork::start() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockNetwork::stop() {
            m_lifecycleState = LifecycleState::STOPPED;
            m_networkStatus = HAL::NetworkStatus::DISCONNECTED;
            return Result<void>();
        }

        Result<void> MockNetwork::suspend() {
            m_lifecycleState = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockNetwork::resume() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockNetwork::shutdown() {
            m_lifecycleState = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        Result<void> MockNetwork::connect(const char* ssid, const char* passphrase) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Network driver not running"));
            }
            if (ssid == nullptr || passphrase == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Credentials cannot be null"));
            }

            m_networkStatus = HAL::NetworkStatus::CONNECTING;
            
            // Simulate successful authentication and connection
            strncpy(m_connectedSSID, ssid, 31);
            m_connectedSSID[31] = '\0';

            m_ipAddress.version = HAL::IPVersion::IPv4;
            m_ipAddress.address.ipv4[0] = 192;
            m_ipAddress.address.ipv4[1] = 168;
            m_ipAddress.address.ipv4[2] = 1;
            m_ipAddress.address.ipv4[3] = 100;

            m_networkStatus = HAL::NetworkStatus::CONNECTED;
            return Result<void>();
        }

        Result<void> MockNetwork::disconnect() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Network driver not running"));
            }
            m_networkStatus = HAL::NetworkStatus::DISCONNECTED;
            m_connectedSSID[0] = '\0';
            return Result<void>();
        }

        Result<int16_t> MockNetwork::getRSSI() const noexcept {
            if (m_networkStatus != HAL::NetworkStatus::CONNECTED) {
                return Result<int16_t>(Error(ErrorCode::INVALID_STATE, "Not connected to network"));
            }
            return Result<int16_t>(-62); // Simulated stable -62 dBm signal
        }

        Result<HAL::IPAddress> MockNetwork::getIPAddress() const noexcept {
            if (m_networkStatus != HAL::NetworkStatus::CONNECTED) {
                return Result<HAL::IPAddress>(Error(ErrorCode::INVALID_STATE, "Not connected to network"));
            }
            return Result<HAL::IPAddress>(m_ipAddress);
        }

    } // namespace Platform
} // namespace DeviceOS
