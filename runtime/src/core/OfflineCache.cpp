#include "../../include/core/OfflineCache.h"
#include <string.h>

namespace DeviceOS {
    namespace Core {

        static uint8_t s_pruneBuffer[64 * 1024];

        OfflineCache::OfflineCache(EventBus& eventBus, ServiceContainer& container) noexcept
            : m_eventBus(eventBus), m_container(container) 
        {
            // Subscribe to state change events and state register changes
            EventFilter stateFilter(Events::SYS_STATE_CHANGED);
            static EventSubscriberNode node1(this, stateFilter);
            m_eventBus.subscribe(node1);

            // Subscribe to telemetry state registers (IDs 0x3000 to 0x300F)
            for (uint16_t id = 0x3000; id <= 0x300F; ++id) {
                EventFilter teleFilter(id);
                static EventSubscriberNode nodes[16] = {
                    EventSubscriberNode(this, EventFilter(0x3000)),
                    EventSubscriberNode(this, EventFilter(0x3001)),
                    EventSubscriberNode(this, EventFilter(0x3002)),
                    EventSubscriberNode(this, EventFilter(0x3003)),
                    EventSubscriberNode(this, EventFilter(0x3004)),
                    EventSubscriberNode(this, EventFilter(0x3005)),
                    EventSubscriberNode(this, EventFilter(0x3006)),
                    EventSubscriberNode(this, EventFilter(0x3007)),
                    EventSubscriberNode(this, EventFilter(0x3008)),
                    EventSubscriberNode(this, EventFilter(0x3009)),
                    EventSubscriberNode(this, EventFilter(0x300A)),
                    EventSubscriberNode(this, EventFilter(0x300B)),
                    EventSubscriberNode(this, EventFilter(0x300C)),
                    EventSubscriberNode(this, EventFilter(0x300D)),
                    EventSubscriberNode(this, EventFilter(0x300E)),
                    EventSubscriberNode(this, EventFilter(0x300F))
                };
                m_eventBus.subscribe(nodes[id - 0x3000]);
            }
        }

        void OfflineCache::onEvent(const Event& event) override {
            if (event.id == Events::SYS_STATE_CHANGED && event.length == 2) {
                uint8_t newState = event.payload[1];
                if (newState == static_cast<uint8_t>(RuntimeState::RUNNING)) {
                    flushToEventBus();
                }
            } else if (event.id >= 0x3000 && event.id <= 0x300F) {
                // Intercept telemetry registers and cache them
                cacheEvent(event);
            }
        }

        Result<void> OfflineCache::cacheEvent(const Event& event) noexcept {
            auto* storage = m_container.get<Platform::MockStorage>(ServiceId::STORAGE);
            if (storage == nullptr) {
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Storage service not found in container"));
            }

            size_t currentSize = getCacheSize();
            size_t requiredSpace = sizeof(CacheRecordHeader) + event.length;

            if (currentSize + requiredSpace > SECTOR_LIMIT) {
                Result<void> pruneRes = pruneOldestRecords(requiredSpace);
                if (pruneRes.isError()) {
                    return pruneRes;
                }
            }

            CacheRecordHeader header;
            header.eventId = event.id;
            header.priority = static_cast<uint8_t>(event.priority);
            header.active = 1;
            header.length = static_cast<uint16_t>(event.length);
            header.timestampMs = event.timestampMs;

            Result<void> writeHeaderRes = storage->writeFileMock(reinterpret_cast<const uint8_t*>(&header), sizeof(header), true);
            if (writeHeaderRes.isError()) return writeHeaderRes;

            if (event.length > 0 && event.payload != nullptr) {
                Result<void> writePayloadRes = storage->writeFileMock(event.payload, event.length, true);
                if (writePayloadRes.isError()) return writePayloadRes;
            }

            return Result<void>();
        }

        Result<void> OfflineCache::pruneOldestRecords(size_t requiredBytes) noexcept {
            auto* storage = m_container.get<Platform::MockStorage>(ServiceId::STORAGE);
            if (storage == nullptr) {
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Storage service not found"));
            }

            size_t fileLen = 0;
            Result<void> readRes = storage->readFileMock(s_pruneBuffer, sizeof(s_pruneBuffer), &fileLen);
            if (readRes.isError()) return readRes;

            size_t freedBytes = 0;
            
            const EventPriority prioritiesToPrune[] = {
                EventPriority::LOW,
                EventPriority::NORMAL,
                EventPriority::HIGH
            };

            for (size_t p = 0; p < 3; ++p) {
                EventPriority targetPriority = prioritiesToPrune[p];
                size_t idx = 0;

                while (idx < fileLen) {
                    CacheRecordHeader* header = reinterpret_cast<CacheRecordHeader*>(&s_pruneBuffer[idx]);
                    
                    if (header->active == 1 && header->priority == static_cast<uint8_t>(targetPriority)) {
                        header->active = 0;
                        freedBytes += sizeof(CacheRecordHeader) + header->length;
                        
                        if (freedBytes >= requiredBytes) {
                            break;
                        }
                    }
                    idx += sizeof(CacheRecordHeader) + header->length;
                }

                if (freedBytes >= requiredBytes) {
                    break;
                }
            }

            if (freedBytes < requiredBytes) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Flash cache limit reached: cannot free space without dropping Critical events"));
            }

            size_t writeIdx = 0;
            size_t readIdx = 0;

            while (readIdx < fileLen) {
                CacheRecordHeader* header = reinterpret_cast<CacheRecordHeader*>(&s_pruneBuffer[readIdx]);
                size_t recordSize = sizeof(CacheRecordHeader) + header->length;

                if (header->active == 1) {
                    if (writeIdx != readIdx) {
                        memmove(&s_pruneBuffer[writeIdx], &s_pruneBuffer[readIdx], recordSize);
                    }
                    writeIdx += recordSize;
                }
                readIdx += recordSize;
            }

            storage->clearFileMock();
            return storage->writeFileMock(s_pruneBuffer, writeIdx, false);
        }

        Result<void> OfflineCache::flushToEventBus() noexcept {
            auto* storage = m_container.get<Platform::MockStorage>(ServiceId::STORAGE);
            if (storage == nullptr) {
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Storage service not found"));
            }

            size_t fileLen = 0;
            Result<void> readRes = storage->readFileMock(s_pruneBuffer, sizeof(s_pruneBuffer), &fileLen);
            if (readRes.isError()) return readRes;

            size_t idx = 0;
            while (idx < fileLen) {
                CacheRecordHeader* header = reinterpret_cast<CacheRecordHeader*>(&s_pruneBuffer[idx]);
                size_t payloadOffset = idx + sizeof(CacheRecordHeader);

                if (header->active == 1) {
                    Event event;
                    event.id = header->eventId;
                    event.priority = static_cast<EventPriority>(header->priority);
                    event.timestampMs = header->timestampMs;
                    event.payload = &s_pruneBuffer[payloadOffset];
                    event.length = header->length;

                    m_eventBus.publishSync(event);
                }
                idx += sizeof(CacheRecordHeader) + header->length;
            }

            clearCache();
            return Result<void>();
        }

        Result<void> OfflineCache::clearCache() noexcept {
            auto* storage = m_container.get<Platform::MockStorage>(ServiceId::STORAGE);
            if (storage == nullptr) {
                return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Storage service not found"));
            }
            storage->clearFileMock();
            return Result<void>();
        }

        size_t OfflineCache::getCacheSize() const noexcept {
            auto* storage = m_container.get<Platform::MockStorage>(ServiceId::STORAGE);
            if (storage == nullptr) {
                return 0;
            }
            size_t len = 0;
            storage->readFileMock(s_pruneBuffer, sizeof(s_pruneBuffer), &len);
            return len;
        }

    } // namespace Core
} // namespace DeviceOS
