#include "../../include/core/StateSync.h"
#include "../../include/core/OfflineCache.h"
#include "../../include/hal/INetwork.h"
#include "../../include/hal/ILogging.h"
#include <string.h>
#include <stdlib.h>

namespace DeviceOS {
    namespace Core {

        StateSync::StateSync(EventBus& eventBus, ServiceContainer& container) noexcept
            : m_eventBus(eventBus), m_container(container)
        {
            for (size_t i = 0; i < REGISTERS_LIMIT; ++i) {
                m_registers[i].key[0] = '\0';
                m_registers[i].type = 0;
                m_registers[i].dirty = false;
                m_registers[i].occupied = false;
            }
        }

        Result<void> StateSync::setInt(const char* key, int32_t val) noexcept {
            StateRegister* reg = findOrAllocate(key);
            if (reg == nullptr) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "State Sync registry is full"));
            }

            if (reg->type != 1 || reg->value.iVal != val || !reg->occupied) {
                reg->value.iVal = val;
                reg->type = 1;
                reg->dirty = true;
            }
            return Result<void>();
        }

        Result<void> StateSync::setFloat(const char* key, float val) noexcept {
            StateRegister* reg = findOrAllocate(key);
            if (reg == nullptr) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "State Sync registry is full"));
            }

            if (reg->type != 2 || reg->value.fVal != val || !reg->occupied) {
                reg->value.fVal = val;
                reg->type = 2;
                reg->dirty = true;
            }
            return Result<void>();
        }

        Result<void> StateSync::setString(const char* key, const char* val) noexcept {
            if (val == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "String value cannot be null"));
            }

            StateRegister* reg = findOrAllocate(key);
            if (reg == nullptr) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "State Sync registry is full"));
            }

            if (reg->type != 3 || strcmp(reg->value.sVal, val) != 0 || !reg->occupied) {
                strncpy(reg->value.sVal, val, 63);
                reg->value.sVal[63] = '\0';
                reg->type = 3;
                reg->dirty = true;
            }
            return Result<void>();
        }

        Result<void> StateSync::sync() noexcept {
            auto* network = m_container.get<HAL::INetwork>(ServiceId::NETWORK);
            auto* logger = m_container.get<HAL::ILogging>(ServiceId::LOGGING);

            bool isOnline = (network != nullptr && network->getStatus() == HAL::NetworkStatus::CONNECTED);

            for (size_t i = 0; i < REGISTERS_LIMIT; ++i) {
                StateRegister& reg = m_registers[i];
                if (reg.occupied && reg.dirty) {
                    if (isOnline) {
                        if (logger != nullptr) {
                            logger->log(HAL::LogLevel::LOG_INFO, "SYNC", 
                                        "Synchronized key '%s' (type %d) directly to Cloud Device Shadow.", 
                                        reg.key, reg.type);
                        }
                        reg.dirty = false;
                    } else {
                        uint8_t payloadBuf[128];
                        size_t payloadLen = 0;

                        payloadBuf[0] = reg.type;
                        payloadLen = 1;

                        if (reg.type == 1) { // Int
                            memcpy(&payloadBuf[1], &reg.value.iVal, sizeof(reg.value.iVal));
                            payloadLen += sizeof(reg.value.iVal);
                        } else if (reg.type == 2) { // Float
                            memcpy(&payloadBuf[1], &reg.value.fVal, sizeof(reg.value.fVal));
                            payloadLen += sizeof(reg.value.fVal);
                        } else if (reg.type == 3) { // String
                            size_t sLen = strlen(reg.value.sVal);
                            if (sLen > 100) sLen = 100;
                            memcpy(&payloadBuf[1], reg.value.sVal, sLen);
                            payloadLen += sLen;
                        }

                        Event event;
                        event.id = 0x3000 + static_cast<uint16_t>(i);
                        event.priority = EventPriority::NORMAL;
                        event.timestampMs = 0;
                        event.payload = payloadBuf;
                        event.length = payloadLen;

                        Result<void> pubRes = m_eventBus.publishSync(event);
                        if (pubRes.isSuccess()) {
                            reg.dirty = false;
                        }
                    }
                }
            }

            return Result<void>();
        }

        Result<void> StateSync::onShadowPatch(const uint8_t* patchPayload, size_t length) noexcept {
            if (patchPayload == nullptr || length == 0) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Payload cannot be empty"));
            }

            const char* json = reinterpret_cast<const char*>(patchPayload);
            size_t idx = 0;

            while (idx < length) {
                if (json[idx] != '"') {
                    idx++;
                    continue;
                }
                idx++; // Skip opening quote
                
                size_t keyStart = idx;
                while (idx < length && json[idx] != '"') {
                    idx++;
                }
                if (idx >= length) break;
                size_t keyLen = idx - keyStart;
                idx++; // Skip closing quote

                while (idx < length && json[idx] != ':') {
                    idx++;
                }
                if (idx >= length) break;
                idx++; // Skip colon

                while (idx < length && (json[idx] == ' ' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) {
                    idx++;
                }
                if (idx >= length) break;

                char key[32];
                if (keyLen >= sizeof(key)) keyLen = sizeof(key) - 1;
                memcpy(key, &json[keyStart], keyLen);
                key[keyLen] = '\0';

                if (json[idx] == '"') {
                    idx++;
                    size_t valStart = idx;
                    while (idx < length && json[idx] != '"') {
                        idx++;
                    }
                    size_t valLen = idx - valStart;
                    idx++;

                    char valStr[64];
                    if (valLen >= sizeof(valStr)) valLen = sizeof(valStr) - 1;
                    memcpy(valStr, &json[valStart], valLen);
                    valStr[valLen] = '\0';

                    setString(key, valStr);
                    StateRegister* reg = findOrAllocate(key);
                    if (reg != nullptr) {
                        reg->dirty = false; // Synced
                    }
                } else if ((json[idx] >= '0' && json[idx] <= '9') || json[idx] == '-' || json[idx] == '.') {
                    size_t valStart = idx;
                    bool isFloat = false;
                    while (idx < length && ((json[idx] >= '0' && json[idx] <= '9') || json[idx] == '-' || json[idx] == '.' || json[idx] == 'e' || json[idx] == 'E')) {
                        if (json[idx] == '.') isFloat = true;
                        idx++;
                    }
                    char valStr[32];
                    size_t valLen = idx - valStart;
                    if (valLen >= sizeof(valStr)) valLen = sizeof(valStr) - 1;
                    memcpy(valStr, &json[valStart], valLen);
                    valStr[valLen] = '\0';

                    if (isFloat) {
                        float fVal = static_cast<float>(atof(valStr));
                        setFloat(key, fVal);
                    } else {
                        int32_t iVal = static_cast<int32_t>(atoi(valStr));
                        setInt(key, iVal);
                    }
                    StateRegister* reg = findOrAllocate(key);
                    if (reg != nullptr) {
                        reg->dirty = false;
                    }
                } else if (json[idx] == 't' || json[idx] == 'f') {
                    bool bVal = (json[idx] == 't');
                    setInt(key, bVal ? 1 : 0);
                    StateRegister* reg = findOrAllocate(key);
                    if (reg != nullptr) {
                        reg->dirty = false;
                    }
                    while (idx < length && json[idx] != ',' && json[idx] != '}') {
                        idx++;
                    }
                }

                // Publish Events::STATE_SHADOW_DELTA_RECEIVED
                Event deltaEvent;
                deltaEvent.id = Events::STATE_SHADOW_DELTA_RECEIVED;
                deltaEvent.priority = EventPriority::HIGH;
                deltaEvent.timestampMs = 0;
                deltaEvent.payload = reinterpret_cast<const uint8_t*>(key);
                deltaEvent.length = strlen(key);
                m_eventBus.publishSync(deltaEvent);
            }

            return Result<void>();
        }

        StateRegister* StateSync::findOrAllocate(const char* key) noexcept {
            if (key == nullptr) return nullptr;

            int freeIdx = -1;
            for (size_t i = 0; i < REGISTERS_LIMIT; ++i) {
                if (m_registers[i].occupied && strcmp(m_registers[i].key, key) == 0) {
                    return &m_registers[i];
                }
                if (!m_registers[i].occupied && freeIdx == -1) {
                    freeIdx = static_cast<int>(i);
                }
            }

            if (freeIdx == -1) return nullptr;

            size_t idx = static_cast<size_t>(freeIdx);
            strncpy(m_registers[idx].key, key, 31);
            m_registers[idx].key[31] = '\0';
            m_registers[idx].occupied = true;
            m_registers[idx].dirty = false;

            return &m_registers[idx];
        }

    } // namespace Core
} // namespace DeviceOS
