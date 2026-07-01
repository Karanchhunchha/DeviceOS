#ifndef DEVICEOS_MOCK_OTA_H
#define DEVICEOS_MOCK_OTA_H

#include "../../include/hal/IOTA.h"

namespace DeviceOS {
    namespace Platform {

        class MockOTA : public HAL::IOTA {
        public:
            MockOTA() noexcept;
            ~MockOTA() override = default;

            // IDriverLifecycle
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_lifecycleState; }

            // IOTA
            Result<void> beginUpdate() override;
            Result<void> writeChunk(const uint8_t* chunk, size_t len) override;
            Result<void> endUpdate() override;
            Result<void> verifySignature(const uint8_t* signature, size_t sigLen, const uint8_t* pubKey, size_t pubKeyLen) override;
            Result<void> applyBootPartition() override;
            Result<void> rollback() override;

        private:
            LifecycleState m_lifecycleState;
            bool m_updateStarted;
            size_t m_bytesWritten;
            uint8_t m_activePartition; // 0 = ota_0, 1 = ota_1
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_OTA_H
