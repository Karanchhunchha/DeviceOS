#ifndef DEVICEOS_HAL_IGPIO_H
#define DEVICEOS_HAL_IGPIO_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        enum class PinMode : uint8_t {
            PIN_INPUT = 0,
            PIN_OUTPUT,
            PIN_INPUT_PULLUP,
            PIN_INPUT_PULLDOWN
        };

        enum class PinState : uint8_t {
            PIN_LOW = 0,
            PIN_HIGH = 1
        };

        enum class InterruptTrigger : uint8_t {
            RISING = 0,
            FALLING,
            CHANGE,
            LOW_LEVEL,
            HIGH_LEVEL
        };

        typedef void (*InterruptCallback)(void* arg);

        /**
         * @brief General Purpose Input/Output Abstraction Layer.
         * Thread Safety: Thread-safe for writing. Reading states should be handled in a safe execution block.
         */
        class IGPIO : public IDriverLifecycle {
        public:
            virtual ~IGPIO() = default;

            /**
             * @brief Configures a pin with the specified operational mode.
             */
            virtual Result<void> setMode(uint8_t pin, PinMode mode) = 0;

            /**
             * @brief Writes a digital state (HIGH or LOW) to an output pin.
             */
            virtual Result<void> write(uint8_t pin, PinState state) = 0;

            /**
             * @brief Reads the current digital state of a pin.
             */
            virtual Result<PinState> read(uint8_t pin) = 0;

            /**
             * @brief Registers an interrupt service routine callback on a specific pin trigger.
             */
            virtual Result<void> attachInterrupt(uint8_t pin, InterruptTrigger trigger, InterruptCallback callback, void* arg) = 0;

            /**
             * @brief Unregisters interrupt handlers from a specific pin.
             */
            virtual Result<void> detachInterrupt(uint8_t pin) = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_IGPIO_H
