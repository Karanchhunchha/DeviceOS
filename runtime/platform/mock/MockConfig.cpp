#include "MockConfig.h"
#include <string.h>

namespace DeviceOS {
    namespace Platform {

        MockConfig::MockConfig() noexcept
            : m_lifecycleState(LifecycleState::UNINITIALIZED)
        {
            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                m_configTable[i].key[0] = '\0';
                m_configTable[i].occupied = false;
                m_configTable[i].type = ConfigEntry::Type::NONE;
            }
        }

        Result<void> MockConfig::initialize() {
            m_lifecycleState = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockConfig::start() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockConfig::stop() {
            m_lifecycleState = LifecycleState::STOPPED;
            return Result<void>();
        }

        Result<void> MockConfig::suspend() {
            m_lifecycleState = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockConfig::resume() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockConfig::shutdown() {
            m_lifecycleState = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        Result<void> MockConfig::setInt(const char* key, int32_t val) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Config driver not running"));
            }
            if (key == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Key cannot be null"));
            }

            int freeIdx = -1;
            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                if (m_configTable[i].occupied && strcmp(m_configTable[i].key, key) == 0) {
                    m_configTable[i].value.iVal = val;
                    m_configTable[i].type = ConfigEntry::Type::INT;
                    return Result<void>();
                }
                if (!m_configTable[i].occupied && freeIdx == -1) {
                    freeIdx = static_cast<int>(i);
                }
            }

            if (freeIdx == -1) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Config registry full"));
            }

            size_t idx = static_cast<size_t>(freeIdx);
            strncpy(m_configTable[idx].key, key, 31);
            m_configTable[idx].key[31] = '\0';
            m_configTable[idx].value.iVal = val;
            m_configTable[idx].type = ConfigEntry::Type::INT;
            m_configTable[idx].occupied = true;

            return Result<void>();
        }

        Result<int32_t> MockConfig::getInt(const char* key, int32_t defaultVal) const noexcept {
            if (key == nullptr) return Result<int32_t>(defaultVal);

            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                if (m_configTable[i].occupied && strcmp(m_configTable[i].key, key) == 0) {
                    if (m_configTable[i].type == ConfigEntry::Type::INT) {
                        return Result<int32_t>(m_configTable[i].value.iVal);
                    }
                }
            }
            return Result<int32_t>(defaultVal);
        }

        Result<void> MockConfig::setFloat(const char* key, float val) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Config driver not running"));
            }
            if (key == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Key cannot be null"));
            }

            int freeIdx = -1;
            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                if (m_configTable[i].occupied && strcmp(m_configTable[i].key, key) == 0) {
                    m_configTable[i].value.fVal = val;
                    m_configTable[i].type = ConfigEntry::Type::FLOAT;
                    return Result<void>();
                }
                if (!m_configTable[i].occupied && freeIdx == -1) {
                    freeIdx = static_cast<int>(i);
                }
            }

            if (freeIdx == -1) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Config registry full"));
            }

            size_t idx = static_cast<size_t>(freeIdx);
            strncpy(m_configTable[idx].key, key, 31);
            m_configTable[idx].key[31] = '\0';
            m_configTable[idx].value.fVal = val;
            m_configTable[idx].type = ConfigEntry::Type::FLOAT;
            m_configTable[idx].occupied = true;

            return Result<void>();
        }

        Result<float> MockConfig::getFloat(const char* key, float defaultVal) const noexcept {
            if (key == nullptr) return Result<float>(defaultVal);

            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                if (m_configTable[i].occupied && strcmp(m_configTable[i].key, key) == 0) {
                    if (m_configTable[i].type == ConfigEntry::Type::FLOAT) {
                        return Result<float>(m_configTable[i].value.fVal);
                    }
                }
            }
            return Result<float>(defaultVal);
        }

        Result<void> MockConfig::setString(const char* key, const char* val) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Config driver not running"));
            }
            if (key == nullptr || val == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Params cannot be null"));
            }

            int freeIdx = -1;
            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                if (m_configTable[i].occupied && strcmp(m_configTable[i].key, key) == 0) {
                    strncpy(m_configTable[i].value.sVal, val, 63);
                    m_configTable[i].value.sVal[63] = '\0';
                    m_configTable[i].type = ConfigEntry::Type::STRING;
                    return Result<void>();
                }
                if (!m_configTable[i].occupied && freeIdx == -1) {
                    freeIdx = static_cast<int>(i);
                }
            }

            if (freeIdx == -1) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Config registry full"));
            }

            size_t idx = static_cast<size_t>(freeIdx);
            strncpy(m_configTable[idx].key, key, 31);
            m_configTable[idx].key[31] = '\0';
            strncpy(m_configTable[idx].value.sVal, val, 63);
            m_configTable[idx].value.sVal[63] = '\0';
            m_configTable[idx].type = ConfigEntry::Type::STRING;
            m_configTable[idx].occupied = true;

            return Result<void>();
        }

        Result<void> MockConfig::getString(const char* key, char* buffer, size_t maxLen) const noexcept {
            if (key == nullptr || buffer == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Params cannot be null"));
            }

            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                if (m_configTable[i].occupied && strcmp(m_configTable[i].key, key) == 0) {
                    if (m_configTable[i].type == ConfigEntry::Type::STRING) {
                        size_t len = strlen(m_configTable[i].value.sVal);
                        if (len >= maxLen) {
                            return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Buffer is too small"));
                        }
                        strcpy(buffer, m_configTable[i].value.sVal);
                        return Result<void>();
                    }
                }
            }

            return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Key not found"));
        }

        Result<void> MockConfig::commit() {
            return Result<void>(); // Commit is a NOP in memory-only mock
        }

        Result<void> MockConfig::eraseAll() {
            for (size_t i = 0; i < CONFIG_LIMIT; ++i) {
                m_configTable[i].occupied = false;
                m_configTable[i].type = ConfigEntry::Type::NONE;
            }
            return Result<void>();
        }

    } // namespace Platform
} // namespace DeviceOS
