#ifndef DEVICEOS_MOCK_SECUREELEMENT_H
#define DEVICEOS_MOCK_SECUREELEMENT_H

#include "../../include/hal/ISecureElement.h"

namespace DeviceOS {
    namespace Platform {

        class MockSecureElement : public HAL::ISecureElement {
        public:
            MockSecureElement() noexcept;
            ~MockSecureElement() override = default;

            // IDriverLifecycle
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_lifecycleState; }

            // ISecureElement
            Result<void> generateCSR(uint8_t* buffer, size_t maxLen, size_t* outLen) override;
            Result<void> getPublicKey(uint8_t* buffer, size_t maxLen, size_t* outLen) override;
            Result<void> signHash(const uint8_t* hash, size_t hashLen, uint8_t* sigBuffer, size_t maxSigLen, size_t* outSigLen) override;
            Result<bool> verifySignature(const uint8_t* hash, size_t hashLen, const uint8_t* signature, size_t sigLen, const uint8_t* pubKey, size_t pubKeyLen) override;

        private:
            LifecycleState m_lifecycleState;
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_SECUREELEMENT_H
