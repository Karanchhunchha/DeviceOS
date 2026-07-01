#include "MockSecureElement.h"
#include <string.h>

namespace DeviceOS {
    namespace Platform {

        MockSecureElement::MockSecureElement() noexcept
            : m_lifecycleState(LifecycleState::UNINITIALIZED) {}

        Result<void> MockSecureElement::initialize() {
            m_lifecycleState = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockSecureElement::start() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockSecureElement::stop() {
            m_lifecycleState = LifecycleState::STOPPED;
            return Result<void>();
        }

        Result<void> MockSecureElement::suspend() {
            m_lifecycleState = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockSecureElement::resume() {
            m_lifecycleState = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockSecureElement::shutdown() {
            m_lifecycleState = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        Result<void> MockSecureElement::generateCSR(uint8_t* buffer, size_t maxLen, size_t* outLen) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Secure Element not running"));
            }
            const char* mockCSR = "MOCK_DEVICE_CSR_DATA";
            size_t len = strlen(mockCSR);
            if (len > maxLen) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "CSR buffer too small"));
            }
            memcpy(buffer, mockCSR, len);
            *outLen = len;
            return Result<void>();
        }

        Result<void> MockSecureElement::getPublicKey(uint8_t* buffer, size_t maxLen, size_t* outLen) {
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Secure Element not running"));
            }
            const char* mockPubKey = "MOCK_DEVICE_PUBLIC_KEY";
            size_t len = strlen(mockPubKey);
            if (len > maxLen) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Public key buffer too small"));
            }
            memcpy(buffer, mockPubKey, len);
            *outLen = len;
            return Result<void>();
        }

        Result<void> MockSecureElement::signHash(const uint8_t* hash, size_t hashLen, uint8_t* sigBuffer, size_t maxSigLen, size_t* outSigLen) {
            (void)hash;
            (void)hashLen;
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Secure Element not running"));
            }
            const char* mockSig = "MOCK_SIGNATURE_64BYTES";
            size_t len = strlen(mockSig);
            if (len > maxSigLen) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Signature buffer too small"));
            }
            memcpy(sigBuffer, mockSig, len);
            *outSigLen = len;
            return Result<void>();
        }

        Result<bool> MockSecureElement::verifySignature(const uint8_t* hash, size_t hashLen, const uint8_t* signature, size_t sigLen, const uint8_t* pubKey, size_t pubKeyLen) {
            (void)hash;
            (void)hashLen;
            if (m_lifecycleState != LifecycleState::RUNNING) {
                return Result<bool>(Error(ErrorCode::INVALID_STATE, "Secure Element not running"));
            }
            if (signature == nullptr || pubKey == nullptr || sigLen == 0 || pubKeyLen == 0) {
                return Result<bool>(Error(ErrorCode::INVALID_ARGUMENT, "Verification params cannot be null"));
            }
            if (sigLen >= 7 && memcmp(signature, "INVALID", 7) == 0) {
                return Result<bool>(false);
            }
            return Result<bool>(true); // Verification succeeds in mock environment
        }

    } // namespace Platform
} // namespace DeviceOS
