#include "MockPower.h"

namespace DeviceOS {
    namespace Platform {

        MockPower::MockPower() noexcept
            : m_lifecycleState(LifecycleState::UNINITIALIZED),
              m_wakeReason(HAL::WakeReason::POWER_ON) {}

        Result<void> MockPower::initialize() {
            m_lifecycleState = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockPower::start() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockPower::stop() {
            m_lifecycleState = LifecycleState::STOPPED;
            return Result<void>();
        }

        Result<void> MockPower::suspend() {
            m_lifecycleState = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockPower::resume() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockPower::shutdown() {
            m_lifecycleState = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        Result<void> MockPower::enterLightSleep(uint64_t durationMs) {
            (void)durationMs;
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Power driver not running"));
            }
            m_wakeReason = HAL::WakeReason::TIMER;
            return Result<void>();
        }

        Result<void> MockPower::enterDeepSleep(uint64_t durationMs) {
            (void)durationMs;
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Power driver not running"));
            }
            m_wakeReason = HAL::WakeReason::TIMER;
            return Result<void>();
        }

        Result<void> MockPower::softwareReset() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Power driver not running"));
            }
            m_wakeReason = HAL::WakeReason::SOFTWARE_RESET;
            return Result<void>();
        }

        Result<float> MockPower::getBatteryVoltage() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<float>(Error(ErrorCode::INVALID_STATE, "Power driver not running"));
            }
            return Result<float>(3.82f); // Simulated stable 3.82V battery
        }

        Result<uint8_t> MockPower::getBatteryPercentage() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<uint8_t>(Error(ErrorCode::INVALID_STATE, "Power driver not running"));
            }
            return Result<uint8_t>(88); // Simulated 88% remaining capacity
        }

    } // namespace Platform
} // namespace DeviceOS
