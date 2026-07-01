#include "MockLogging.h"
#include <stdio.h>

namespace DeviceOS {
    namespace Platform {

        MockLogging::MockLogging() noexcept
            : m_lifecycleState(LifecycleState::UNINITIALIZED) {}

        Result<void> MockLogging::initialize() {
            m_lifecycleState = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockLogging::start() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockLogging::stop() {
            m_lifecycleState = LifecycleState::STOPPED;
            return Result<void>();
        }

        Result<void> MockLogging::suspend() {
            m_lifecycleState = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockLogging::resume() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockLogging::shutdown() {
            m_lifecycleState = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        void MockLogging::log(HAL::LogLevel level, const char* tag, const char* format, ...) {
            if (m_lifecycleState != LifecycleState::RUNNING) return;

            va_list args;
            va_start(args, format);
            logVarArgs(level, tag, format, args);
            va_end(args);
        }

        void MockLogging::logVarArgs(HAL::LogLevel level, const char* tag, const char* format, va_list args) {
            if (m_lifecycleState != LifecycleState::RUNNING) return;

            // Output format: [LEVEL][TAG] message
            printf("[%s][%s] ", getLevelName(level), tag ? tag : "SYS");
            vprintf(format, args);
            printf("\n");
        }

        void MockLogging::logRaw(HAL::LogLevel level, const char* tag, const uint8_t* data, size_t len) {
            if (m_lifecycleState != LifecycleState::RUNNING) return;

            printf("[%s][%s] [RAW: %zu bytes] ", getLevelName(level), tag ? tag : "SYS", len);
            for (size_t i = 0; i < len; ++i) {
                printf("%02X ", data[i]);
            }
            printf("\n");
        }

        const char* MockLogging::getLevelName(HAL::LogLevel level) noexcept {
            switch (level) {
                case HAL::LogLevel::LOG_DEBUG: return "DEBUG";
                case HAL::LogLevel::LOG_INFO:  return "INFO";
                case HAL::LogLevel::LOG_WARN:  return "WARN";
                case HAL::LogLevel::LOG_ERROR: return "ERROR";
                default:                       return "LOG";
            }
        }

    } // namespace Platform
} // namespace DeviceOS
