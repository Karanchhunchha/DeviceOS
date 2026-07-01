#ifndef DEVICEOS_CORE_OTAMANAGER_H
#define DEVICEOS_CORE_OTAMANAGER_H

#include "Result.h"
#include "EventBus.h"
#include "ServiceContainer.h"

namespace DeviceOS {

    enum class OTAState : uint8_t {
        IDLE = 0,
        DOWNLOADING,
        VERIFYING,
        APPLYING,
        ROLLBACK
    };

    namespace Core {

        /**
         * @brief Dual-Partition Secure Boot OTA Updater.
         * Coordinates binary ingestion streams, verifies cryptographic signatures, and swaps partitions.
         */
        class OTAManager {
        public:
            OTAManager(EventBus& eventBus, ServiceContainer& container) noexcept;
            ~OTAManager() = default;

            // Disable copy
            OTAManager(const OTAManager&) = delete;
            OTAManager& operator=(const OTAManager&) = delete;

            /**
             * @brief Starts an OTA update session.
             */
            Result<void> beginUpdate() noexcept;

            /**
             * @brief Ingests and writes a single firmware binary chunk.
             * Computes rolling hashes internally.
             */
            Result<void> writeChunk(const uint8_t* chunk, size_t len) noexcept;

            /**
             * @brief Concludes update session and validates signature.
             * Swaps active partition upon successful verification.
             */
            Result<void> verifyAndApply(const uint8_t* signature, size_t sigLen) noexcept;

            /**
             * @brief Reverts boot flags back to the previous stable partition.
             */
            Result<void> rollback() noexcept;

            OTAState getState() const noexcept { return m_state; }

        private:
            EventBus& m_eventBus;
            ServiceContainer& m_container;
            OTAState m_state;
            size_t m_bytesWritten;

            // Simulated rolling SHA-256 state tracking (xor checksum of bytes)
            uint32_t m_rollingChecksum;
        };

    } // namespace Core
} // namespace DeviceOS

#endif // DEVICEOS_CORE_OTAMANAGER_H
