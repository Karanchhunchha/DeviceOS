#include "MockBluetooth.h"
#include <string.h>

namespace DeviceOS {
    namespace Platform {

        MockBluetooth::MockBluetooth() noexcept
            : m_lifecycleState(LifecycleState::UNINITIALIZED),
              m_bleState(HAL::BLEState::IDLE)
        {
            m_advertisedName[0] = '\0';
        }

        Result<void> MockBluetooth::initialize() {
            m_lifecycleState = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockBluetooth::start() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockBluetooth::stop() {
            m_lifecycleState = LifecycleState::STOPPED;
            m_bleState = HAL::BLEState::IDLE;
            return Result<void>();
        }

        Result<void> MockBluetooth::suspend() {
            m_lifecycleState = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockBluetooth::resume() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockBluetooth::shutdown() {
            m_lifecycleState = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        Result<void> MockBluetooth::registerGATTService(const HAL::GATTService& service) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Bluetooth driver not running"));
            }
            if (service.uuid == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Service UUID cannot be null"));
            }
            return Result<void>();
        }

        Result<void> MockBluetooth::startAdvertising(const char* deviceName) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Bluetooth driver not running"));
            }
            if (deviceName == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Device name cannot be null"));
            }

            m_bleState = HAL::BLEState::ADVERTISING;
            strncpy(m_advertisedName, deviceName, 31);
            m_advertisedName[31] = '\0';
            return Result<void>();
        }

        Result<void> MockBluetooth::stopAdvertising() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Bluetooth driver not running"));
            }
            m_bleState = HAL::BLEState::IDLE;
            m_advertisedName[0] = '\0';
            return Result<void>();
        }

        Result<void> MockBluetooth::notifyCharacteristic(const char* charUuid, const uint8_t* data, size_t len) {
            (void)charUuid;
            (void)data;
            (void)len;
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Bluetooth driver not running"));
            }
            return Result<void>();
        }

    } // namespace Platform
} // namespace DeviceOS
