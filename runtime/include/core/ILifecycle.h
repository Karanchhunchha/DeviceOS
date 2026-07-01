#ifndef DEVICEOS_CORE_ILIFECYCLE_H
#define DEVICEOS_CORE_ILIFECYCLE_H

#include "Result.h"

namespace DeviceOS {

    /**
     * @brief States representing the standard operational lifecycle of drivers and modules.
     */
    enum class LifecycleState {
        UNINITIALIZED = 0,
        INITIALIZED,
        RUNNING,
        STOPPED,
        SUSPENDED
    };

    /**
     * @brief Interface contract to standardize driver initialization, execution control, and power management.
     */
    class IDriverLifecycle {
    public:
        virtual ~IDriverLifecycle() = default;

        /**
         * @brief Allocates hardware/software resources and transitions to INITIALIZED.
         * Must be non-blocking and idempotent.
         */
        virtual Result<void> initialize() = 0;

        /**
         * @brief Starts driver execution, transitions state to RUNNING.
         * Enables interrupts, schedules tasks, or registers event hooks.
         */
        virtual Result<void> start() = 0;

        /**
         * @brief Stops driver execution, transitions state to STOPPED.
         * Disables interrupts and stops tasks to conserve resources.
         */
        virtual Result<void> stop() = 0;

        /**
         * @brief Places driver into low-power mode, transitioning state to SUSPENDED.
         * Must preserve state registers.
         */
        virtual Result<void> suspend() = 0;

        /**
         * @brief Restores driver from low-power mode, transitioning state to RUNNING.
         */
        virtual Result<void> resume() = 0;

        /**
         * @brief Deallocates all resources, transitioning back to UNINITIALIZED.
         */
        virtual Result<void> shutdown() = 0;

        /**
         * @brief Retrieves the current state of the driver.
         */
        virtual LifecycleState getState() const noexcept = 0;
    };

} // namespace DeviceOS

#endif // DEVICEOS_CORE_ILIFECYCLE_H
