#include "MockStorage.h"
#include <string.h>

namespace DeviceOS {
    namespace Platform {

        MockStorage::MockStorage() noexcept
            : m_state(LifecycleState::UNINITIALIZED),
              m_filePointer(0),
              m_fsMounted(false),
              m_writeCyclesCount(0)
        {
            // Zero out tables
            for (size_t i = 0; i < NVS_SLOTS_LIMIT; ++i) {
                m_nvsTable[i].namespaceName[0] = '\0';
                m_nvsTable[i].key[0] = '\0';
                m_nvsTable[i].len = 0;
                m_nvsTable[i].occupied = false;
            }
            memset(m_flashMemory, 0, FLASH_SIZE_LIMIT);
        }

        Result<void> MockStorage::initialize() {
            m_state = LifecycleState::INITIALIZED;
            return Result<void>();
        }

        Result<void> MockStorage::start() {
            m_state = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockStorage::stop() {
            m_state = LifecycleState::STOPPED;
            return Result<void>();
        }

        Result<void> MockStorage::suspend() {
            m_state = LifecycleState::SUSPENDED;
            return Result<void>();
        }

        Result<void> MockStorage::resume() {
            m_state = LifecycleState::RUNNING;
            return Result<void>();
        }

        Result<void> MockStorage::shutdown() {
            m_state = LifecycleState::UNINITIALIZED;
            return Result<void>();
        }

        Result<void> MockStorage::write(const char* namespaceName, const char* key, const uint8_t* val, size_t len) {
            if (m_state != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Storage driver is not running"));
            }
            if (namespaceName == nullptr || key == nullptr || val == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Write parameters cannot be null"));
            }
            if (len > 128) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Value size exceeds mock 128-byte slot limit"));
            }

            int freeSlot = -1;
            for (size_t i = 0; i < NVS_SLOTS_LIMIT; ++i) {
                if (m_nvsTable[i].occupied && 
                    strcmp(m_nvsTable[i].namespaceName, namespaceName) == 0 &&
                    strcmp(m_nvsTable[i].key, key) == 0) 
                {
                    // Found existing key; overwrite
                    memcpy(m_nvsTable[i].value, val, len);
                    m_nvsTable[i].len = len;
                    m_writeCyclesCount++;
                    return Result<void>();
                }
                if (!m_nvsTable[i].occupied && freeSlot == -1) {
                    freeSlot = static_cast<int>(i);
                }
            }

            if (freeSlot == -1) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Mock NVS table slots are full"));
            }

            // Write to free slot
            size_t idx = static_cast<size_t>(freeSlot);
            strncpy(m_nvsTable[idx].namespaceName, namespaceName, 31);
            m_nvsTable[idx].namespaceName[31] = '\0';
            strncpy(m_nvsTable[idx].key, key, 31);
            m_nvsTable[idx].key[31] = '\0';
            memcpy(m_nvsTable[idx].value, val, len);
            m_nvsTable[idx].len = len;
            m_nvsTable[idx].occupied = true;
            m_writeCyclesCount++;

            return Result<void>();
        }

        Result<void> MockStorage::read(const char* namespaceName, const char* key, uint8_t* buffer, size_t maxLen, size_t* outLen) {
            if (m_state != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Storage driver is not running"));
            }
            if (namespaceName == nullptr || key == nullptr || buffer == nullptr || outLen == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Read parameters cannot be null"));
            }

            for (size_t i = 0; i < NVS_SLOTS_LIMIT; ++i) {
                if (m_nvsTable[i].occupied &&
                    strcmp(m_nvsTable[i].namespaceName, namespaceName) == 0 &&
                    strcmp(m_nvsTable[i].key, key) == 0)
                {
                    if (m_nvsTable[i].len > maxLen) {
                        return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Read buffer is too small"));
                    }
                    memcpy(buffer, m_nvsTable[i].value, m_nvsTable[i].len);
                    *outLen = m_nvsTable[i].len;
                    return Result<void>();
                }
            }

            return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Key not found in mock NVS registry"));
        }

        Result<void> MockStorage::remove(const char* namespaceName, const char* key) {
            if (m_state != LifecycleState::RUNNING) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Storage driver is not running"));
            }
            if (namespaceName == nullptr || key == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Remove parameters cannot be null"));
            }

            for (size_t i = 0; i < NVS_SLOTS_LIMIT; ++i) {
                if (m_nvsTable[i].occupied &&
                    strcmp(m_nvsTable[i].namespaceName, namespaceName) == 0 &&
                    strcmp(m_nvsTable[i].key, key) == 0)
                {
                    m_nvsTable[i].occupied = false;
                    m_writeCyclesCount++;
                    return Result<void>();
                }
            }

            return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Key not found in mock NVS registry"));
        }

        Result<void> MockStorage::mountFilesystem(const char* partitionName, const char* mountPoint) {
            (void)partitionName;
            (void)mountPoint;
            m_fsMounted = true;
            return Result<void>();
        }

        Result<void> MockStorage::unmountFilesystem(const char* mountPoint) {
            (void)mountPoint;
            m_fsMounted = false;
            return Result<void>();
        }

        Result<uint32_t> MockStorage::getEstimatedWearCyclesRemaining() {
            if (m_writeCyclesCount >= MAX_WRITE_CYCLES) {
                return Result<uint32_t>(0);
            }
            return Result<uint32_t>(MAX_WRITE_CYCLES - m_writeCyclesCount);
        }

        Result<void> MockStorage::writeFileMock(const uint8_t* data, size_t len, bool append) noexcept {
            if (!m_fsMounted) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Filesystem not mounted"));
            }
            if (data == nullptr && len > 0) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Write data cannot be null"));
            }

            if (!append) {
                m_filePointer = 0;
            }

            if (m_filePointer + len > FLASH_SIZE_LIMIT) {
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Mock flash partition file limits exceeded"));
            }

            if (len > 0) {
                memcpy(&m_flashMemory[m_filePointer], data, len);
                m_filePointer += len;
                m_writeCyclesCount++;
            }

            return Result<void>();
        }

        Result<void> MockStorage::readFileMock(uint8_t* buffer, size_t maxLen, size_t* outLen) const noexcept {
            if (!m_fsMounted) {
                return Result<void>(Error(ErrorCode::INVALID_STATE, "Filesystem not mounted"));
            }
            if (buffer == nullptr || outLen == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Read buffers cannot be null"));
            }

            if (m_filePointer > maxLen) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Buffer is too small to read mock file"));
            }

            if (m_filePointer > 0) {
                memcpy(buffer, m_flashMemory, m_filePointer);
            }
            *outLen = m_filePointer;
            return Result<void>();
        }

        void MockStorage::clearFileMock() noexcept {
            m_filePointer = 0;
        }

    } // namespace Platform
} // namespace DeviceOS
