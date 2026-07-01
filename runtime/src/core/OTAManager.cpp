#include "../../include/core/OTAManager.h"
#include "../../include/hal/IOTA.h"
#include "../../include/hal/ISecureElement.h"
#include "../../include/core/EventTypes.h"
#include <string.h>

namespace DeviceOS {
    namespace Core {

        OTAManager::OTAManager(EventBus& eventBus, ServiceContainer& container) noexcept
            : m_eventBus(eventBus), 
              m_container(container), 
              m_state(OTAState::IDLE), 
              m_bytesWritten(0), 
              m_rollingChecksum(0) {}

        Result<void> OTAManager::beginUpdate() noexcept {
            if (m_state != OTAState::IDLE) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA update already in progress"));
            }

            auto* otaDriver = m_container.get<HAL::IOTA>(ServiceId::OTA);
            if (otaDriver == nullptr) {
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "OTA driver not registered"));
            }

            Result<void> res = otaDriver->beginUpdate();
            if (res.isError()) return res;

            m_state = OTAState::DOWNLOADING;
            m_bytesWritten = 0;
            m_rollingChecksum = 0;
            return Result<void>();
        }

        Result<void> OTAManager::writeChunk(const uint8_t* chunk, size_t len) noexcept {
            if (m_state != OTAState::DOWNLOADING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA not in downloading state"));
            }

            auto* otaDriver = m_container.get<HAL::IOTA>(ServiceId::OTA);
            if (otaDriver == nullptr) {
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "OTA driver not registered"));
            }

            Result<void> res = otaDriver->writeChunk(chunk, len);
            if (res.isError()) return res;

            // Compute rolling simple xor checksum (simulated SHA-256 rolling digest)
            for (size_t i = 0; i < len; ++i) {
                m_rollingChecksum ^= chunk[i];
                // Rotate checksum bits to simulate hash mixing
                m_rollingChecksum = (m_rollingChecksum << 1) | (m_rollingChecksum >> 31);
            }

            m_bytesWritten += len;
            return Result<void>();
        }

        Result<void> OTAManager::verifyAndApply(const uint8_t* signature, size_t sigLen) noexcept {
            if (m_state != OTAState::DOWNLOADING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA not in downloading state"));
            }

            m_state = OTAState::VERIFYING;

            auto* otaDriver = m_container.get<HAL::IOTA>(ServiceId::OTA);
            if (otaDriver == nullptr) {
                m_state = OTAState::IDLE;
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "OTA driver not registered"));
            }

            Result<void> endRes = otaDriver->endUpdate();
            if (endRes.isError()) {
                m_state = OTAState::IDLE;
                return endRes;
            }

            auto* secureDriver = m_container.get<HAL::ISecureElement>(ServiceId::SECURE_ELEMENT);
            if (secureDriver == nullptr) {
                m_state = OTAState::IDLE;
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Secure element driver not registered"));
            }

            // Obtain device public key
            uint8_t pubKey[64];
            size_t pubKeyLen = 0;
            Result<void> keyRes = secureDriver->getPublicKey(pubKey, sizeof(pubKey), &pubKeyLen);
            if (keyRes.isError()) {
                m_state = OTAState::IDLE;
                return keyRes;
            }

            // Format simulated 32-byte hash out of m_rollingChecksum
            uint8_t simulatedHash[32];
            memset(simulatedHash, 0, sizeof(simulatedHash));
            simulatedHash[0] = static_cast<uint8_t>(m_rollingChecksum >> 24);
            simulatedHash[1] = static_cast<uint8_t>(m_rollingChecksum >> 16);
            simulatedHash[2] = static_cast<uint8_t>(m_rollingChecksum >> 8);
            simulatedHash[3] = static_cast<uint8_t>(m_rollingChecksum);

            // Cryptographically verify firmware signature using secure crypt chip
            Result<bool> verifyRes = secureDriver->verifySignature(
                simulatedHash, sizeof(simulatedHash),
                signature, sigLen,
                pubKey, pubKeyLen
            );

            if (verifyRes.isError() || !verifyRes.unwrap()) {
                m_state = OTAState::IDLE;
                return Result<void>(Error(ErrorCode::SECURITY_VIOLATION, "Firmware signature validation failed"));
            }

            m_state = OTAState::APPLYING;

            // Apply new boot partition flags
            Result<void> applyRes = otaDriver->applyBootPartition();
            if (applyRes.isError()) {
                m_state = OTAState::IDLE;
                return applyRes;
            }

            // Broadcast OTA start update event
            Event event;
            event.id = Events::DEVICE_OTA_START;
            event.priority = EventPriority::CRITICAL;
            event.timestampMs = 0;
            event.payload = nullptr;
            event.length = 0;
            m_eventBus.publishSync(event);

            m_state = OTAState::IDLE;
            return Result<void>();
        }

        Result<void> OTAManager::rollback() noexcept {
            m_state = OTAState::ROLLBACK;

            auto* otaDriver = m_container.get<HAL::IOTA>(ServiceId::OTA);
            if (otaDriver == nullptr) {
                m_state = OTAState::IDLE;
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "OTA driver not registered"));
            }

            Result<void> res = otaDriver->rollback();
            m_state = OTAState::IDLE;
            return res;
        }

    } // namespace Core
} // namespace DeviceOS
