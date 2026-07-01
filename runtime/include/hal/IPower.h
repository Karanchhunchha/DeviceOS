#ifndef DEVICEOS_HAL_IPOWER_H
#define DEVICEOS_HAL_IPOWER_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        enum class WakeReason : uint8_t {
            POWER_ON = 0,
            EXTERNAL_PIN,
            TIMER,
            BLE_WAKE,
            SOFTWARE_RESET,
            UNKNOWN
        };

        /**
         * @brief System Power Management and Low-Power Mode Abstraction.
         */
        class IPower : public IDriverLifecycle {
        public:
            virtual ~IPower() = default;

            /**
             * @brief Suspends execution, placing RAM into self-refresh mode and shutting down radio peripherals.
             * Wakes up when duration expires or on registered GPIO triggers.
             */
            virtual Result<void> enterLightSleep(uint64_t durationMs) = 0;

            /**
             * @brief Powers off CPUs, RAM, and radios. Wake-up triggers system reboot.
             */
            virtual Result<void> enterDeepSleep(uint64_t durationMs) = 0;

            /**
             * @brief Evaluates why the microcontroller rebooted or woke up.
             */
            virtual WakeReason getWakeReason() const noexcept = 0;

            /**
             * @brief Triggers an immediate hardware reset.
             */
            virtual Result<void> softwareReset() = 0;

            /**
             * @brief Retrieves live system battery voltage if supported.
             */
            virtual Result<float> getBatteryVoltage() = 0;

            /**
             * @brief Retrieves current battery charge capacity (0 to 100).
             */
            virtual Result<uint8_t> getBatteryPercentage() = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_IPOWER_H
