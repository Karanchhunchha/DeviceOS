#ifndef DEVICEOS_MOCK_CONFIG_H
#define DEVICEOS_MOCK_CONFIG_H

#include "../../include/hal/IConfig.h"

namespace DeviceOS {
    namespace Platform {

        struct ConfigEntry {
            char key[32];
            union {
                int32_t iVal;
                float fVal;
                char sVal[64];
            } value;
            enum class Type {
                NONE = 0,
                INT,
                FLOAT,
                STRING
            } type;
            bool occupied;
        };

        class MockConfig : public HAL::IConfig {
        public:
            MockConfig() noexcept;
            ~MockConfig() override = default;

            // IDriverLifecycle
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_lifecycleState; }

            // IConfig
            Result<void> setInt(const char* key, int32_t val) override;
            Result<int32_t> getInt(const char* key, int32_t defaultVal) const noexcept override;

            Result<void> setFloat(const char* key, float val) override;
            Result<float> getFloat(const char* key, float defaultVal) const noexcept override;

            Result<void> setString(const char* key, const char* val) override;
            Result<void> getString(const char* key, char* buffer, size_t maxLen) const noexcept override;

            Result<void> commit() override;
            Result<void> eraseAll() override;

        private:
            LifecycleState m_lifecycleState;
            static constexpr size_t CONFIG_LIMIT = 16;
            ConfigEntry m_configTable[CONFIG_LIMIT];
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_CONFIG_H
