#ifndef DEVICEOS_HAL_IOTA_H
#define DEVICEOS_HAL_IOTA_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        /**
         * @brief Over-The-Air (OTA) Firmware Upgrade Interface.
         */
        class IOTA : public IDriverLifecycle {
        public:
            virtual ~IOTA() = default;

            /**
             * @brief Prepares the inactive OTA flash partition to receive dynamic firmware data.
             */
            virtual Result<void> beginUpdate() = 0;

            /**
             * @brief Appends a contiguous block of downloaded firmware to flash.
             */
            virtual Result<void> writeChunk(const uint8_t* chunk, size_t len) = 0;

            /**
             * @brief Closes the update partition and flushes pending writes to SPI flash.
             */
            virtual Result<void> endUpdate() = 0;

            /**
             * @brief Verifies firmware block signatures using an on-device public key.
             */
            virtual Result<void> verifySignature(const uint8_t* signature, size_t sigLen, const uint8_t* pubKey, size_t pubKeyLen) = 0;

            /**
             * @brief Sets the inactive partition as active boot target.
             * Swaps boot partition registers in the next boot phase.
             */
            virtual Result<void> applyBootPartition() = 0;

            /**
             * @brief Forces bootloader to roll back to the previously stable active partition.
             */
            virtual Result<void> rollback() = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_IOTA_H
