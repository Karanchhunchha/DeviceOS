#ifndef DEVICEOS_HAL_ISTORAGE_H
#define DEVICEOS_HAL_ISTORAGE_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        /**
         * @brief Non-Volatile Flash Memory and Filesystem Abstraction Layer.
         */
        class IStorage : public IDriverLifecycle {
        public:
            virtual ~IStorage() = default;

            /**
             * @brief Writes a binary blob to a named key-value namespace (NVS).
             */
            virtual Result<void> write(const char* namespaceName, const char* key, const uint8_t* val, size_t len) = 0;

            /**
             * @brief Reads a binary blob from a named key-value namespace (NVS).
             */
            virtual Result<void> read(const char* namespaceName, const char* key, uint8_t* buffer, size_t maxLen, size_t* outLen) = 0;

            /**
             * @brief Deletes a key from the named namespace.
             */
            virtual Result<void> remove(const char* namespaceName, const char* key) = 0;

            /**
             * @brief Mounts a file partition (e.g., SPIFFS, LittleFS) into the virtual file system (VFS).
             */
            virtual Result<void> mountFilesystem(const char* partitionName, const char* mountPoint) = 0;

            /**
             * @brief Unmounts a mounted file system partition.
             */
            virtual Result<void> unmountFilesystem(const char* mountPoint) = 0;

            /**
             * @brief Diagnoses storage reliability by estimating remaining write/erase endurance.
             */
            virtual Result<uint32_t> getEstimatedWearCyclesRemaining() = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_ISTORAGE_H
