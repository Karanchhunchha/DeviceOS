#ifndef DEVICEOS_CORE_OFFLINECACHE_H
#define DEVICEOS_CORE_OFFLINECACHE_H

#include "EventBus.h"
#include "ServiceContainer.h"
#include "../../platform/mock/MockStorage.h"

namespace DeviceOS {

    /**
     * @brief Fixed-size header preceding every persisted telemetry frame in flash storage.
     */
    struct CacheRecordHeader {
        uint16_t eventId;
        uint8_t priority;
        uint8_t active;      // 1 = active, 0 = flagged/pruned
        uint16_t length;     // payload byte count
        uint32_t timestampMs;
    };

    namespace Core {

        /**
         * @brief Offline-First Persistence Manager Engine.
         * Intercepts telemetry events and writes them to flash storage, pruning by priority if full.
         */
        class OfflineCache : public IEventSubscriber {
        public:
            OfflineCache(EventBus& eventBus, ServiceContainer& container) noexcept;
            ~OfflineCache() override = default;

            // IEventSubscriber override
            void onEvent(const Event& event) override;

            /**
             * @brief Serializes and writes a telemetry event to mock flash partitions.
             */
            Result<void> cacheEvent(const Event& event) noexcept;

            /**
             * @brief Reads all stored active events, republishes them to the Event Bus, and clears storage.
             */
            Result<void> flushToEventBus() noexcept;

            /**
             * @brief Clears the active telemetry partition file.
             */
            Result<void> clearCache() noexcept;

            /**
             * @brief Returns current active byte count stored.
             */
            size_t getCacheSize() const noexcept;

        private:
            EventBus& m_eventBus;
            ServiceContainer& m_container;

            static constexpr const char* MOUNT_PATH = "/spiflash/telemetry.bin";
            static constexpr size_t SECTOR_LIMIT = 64 * 1024; // 64KB log file size cap

            /**
             * @brief Scans and prunes records of target priority levels to recover space.
             */
            Result<void> pruneOldestRecords(size_t requiredBytes) noexcept;
        };

    } // namespace Core
} // namespace DeviceOS

#endif // DEVICEOS_CORE_OFFLINECACHE_H
