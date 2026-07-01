#ifndef DEVICEOS_MOCK_STORAGE_H
#define DEVICEOS_MOCK_STORAGE_H

#include "../../include/hal/IStorage.h"

namespace DeviceOS {
    namespace Platform {

        struct KVEntry {
            char namespaceName[32];
            char key[32];
            uint8_t value[128];
            size_t len;
            bool occupied;
        };

        /**
         * @brief Simulated Flash Storage and Filesystem Driver for Simulation Targets.
         * Enforces strict memory boundaries and tracks write cycles.
         */
        class MockStorage : public HAL::IStorage {
        public:
            MockStorage() noexcept;
            ~MockStorage() override = default;

            // IDriverLifecycle interfaces
            Result<void> initialize() override;
            Result<void> start() override;
            Result<void> stop() override;
            Result<void> suspend() override;
            Result<void> resume() override;
            Result<void> shutdown() override;
            LifecycleState getState() const noexcept override { return m_state; }

            // IStorage interfaces
            Result<void> write(const char* namespaceName, const char* key, const uint8_t* val, size_t len) override;
            Result<void> read(const char* namespaceName, const char* key, uint8_t* buffer, size_t maxLen, size_t* outLen) override;
            Result<void> remove(const char* namespaceName, const char* key) override;

            Result<void> mountFilesystem(const char* partitionName, const char* mountPoint) override;
            Result<void> unmountFilesystem(const char* mountPoint) override;

            Result<uint32_t> getEstimatedWearCyclesRemaining() override;

            // Direct simulation methods for file-like mock checks
            Result<void> writeFileMock(const uint8_t* data, size_t len, bool append = true) noexcept;
            Result<void> readFileMock(uint8_t* buffer, size_t maxLen, size_t* outLen) const noexcept;
            void clearFileMock() noexcept;

        private:
            LifecycleState m_state;
            
            // Fixed key-value slots (NVS simulation)
            static constexpr size_t NVS_SLOTS_LIMIT = 32;
            KVEntry m_nvsTable[NVS_SLOTS_LIMIT];

            // Simulated raw flash partition for virtual file operations (telemetry log cache)
            static constexpr size_t FLASH_SIZE_LIMIT = 64 * 1024; // 64KB mock flash sector
            uint8_t m_flashMemory[FLASH_SIZE_LIMIT];
            size_t m_filePointer;
            bool m_fsMounted;

            uint32_t m_writeCyclesCount;
            static constexpr uint32_t MAX_WRITE_CYCLES = 100000; // standard flash life
        };

    } // namespace Platform
} // namespace DeviceOS

#endif // DEVICEOS_MOCK_STORAGE_H
