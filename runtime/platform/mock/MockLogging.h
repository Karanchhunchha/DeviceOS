#ifndef DEVICEOS_MOCK_LOGGING_H
#define DEVICEOS_MOCK_LOGGING_H

#include "../../include/hal/ILogging.h"

namespace DeviceOS {
    namespace Platform {

        class MockLogging : public HAL::ILogging {
        public:
            MockLogging() noexcept;
            ~MockLogging() override = default;

            // IDriverLifecycle
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_lifecycleState; }

            // ILogging
            void log(HAL::LogLevel level, const char* tag, const char* format, ...) override;
            void logVarArgs(HAL::LogLevel level, const char* tag, const char* format, va_list args) override;
            void logRaw(HAL::LogLevel level, const char* tag, const uint8_t* data, size_t len) override;

        private:
            LifecycleState m_lifecycleState;
            static const char* getLevelName(HAL::LogLevel level) noexcept;
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_LOGGING_H
