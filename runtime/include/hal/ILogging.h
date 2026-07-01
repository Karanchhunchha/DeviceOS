#ifndef DEVICEOS_HAL_ILOGGING_H
#define DEVICEOS_HAL_ILOGGING_H

#include "../core/ILifecycle.h"
#include <stdarg.h>

namespace DeviceOS {
    namespace HAL {

        enum class LogLevel : uint8_t {
            LOG_DEBUG = 0,
            LOG_INFO,
            LOG_WARN,
            LOG_ERROR
        };

        /**
         * @brief Structured logging and debugging telemetry transport interface.
         */
        class ILogging : public IDriverLifecycle {
        public:
            virtual ~ILogging() = default;

            /**
             * @brief Writes a formatted text log entry.
             * Must format using stack-allocated buffers to prevent heap allocations.
             */
            virtual void log(LogLevel level, const char* tag, const char* format, ...) = 0;

            /**
             * @brief Low-level varargs log entry formatter.
             */
            virtual void logVarArgs(LogLevel level, const char* tag, const char* format, va_list args) = 0;

            /**
             * @brief Logs raw data packets (e.g. stack traces, heap dump frames) directly.
             */
            virtual void logRaw(LogLevel level, const char* tag, const uint8_t* data, size_t len) = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_ILOGGING_H
