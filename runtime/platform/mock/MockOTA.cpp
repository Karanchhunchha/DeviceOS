#include "MockOTA.h"

namespace DeviceOS {
    namespace Platform {

        MockOTA::MockOTA() noexcept
            : m_lifecycleState(LifecycleState::UNINITIALIZED),
              m_updateStarted(false),
              m_bytesWritten(0),
              m_activePartition(0) {}

        Result<void> MockOTA::initialize() {
            m_lifecycleState = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockOTA::start() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockOTA::stop() {
            m_lifecycleState = LifecycleState::STOPPED;
            m_updateStarted = false;
            return Result<void>();
        }

        Result<void> MockOTA::suspend() {
            m_lifecycleState = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockOTA::resume() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockOTA::shutdown() {
            m_lifecycleState = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        Result<void> MockOTA::beginUpdate() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA driver not running"));
            }
            m_bytesWritten = 0;
            m_updateStarted = true;
            return Result<void>();
        }

        Result<void> MockOTA::writeChunk(const uint8_t* chunk, size_t len) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA driver not running"));
            }
            if (!m_updateStarted) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA session not initiated"));
            }
            if (chunk == nullptr && len > 0) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "OTA chunk data cannot be null"));
            }

            m_bytesWritten += len;
            return Result<void>();
        }

        Result<void> MockOTA::endUpdate() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA driver not running"));
            }
            if (!m_updateStarted) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA session not initiated"));
            }
            m_updateStarted = false;
            return Result<void>();
        }

        Result<void> MockOTA::verifySignature(const uint8_t* signature, size_t sigLen, const uint8_t* pubKey, size_t pubKeyLen) {
            (void)signature;
            (void)sigLen;
            (void)pubKey;
            (void)pubKeyLen;
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA driver not running"));
            }
            return Result<void>(); // Verification succeeds in mock
        }

        Result<void> MockOTA::applyBootPartition() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA driver not running"));
            }
            // Swap active partition flag
            m_activePartition = 1 - m_activePartition;
            return Result<void>();
        }

        Result<void> MockOTA::rollback() {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "OTA driver not running"));
            }
            m_activePartition = 1 - m_activePartition;
            return Result<void>();
        }

    } // namespace Platform
} // namespace DeviceOS
