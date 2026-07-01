#ifndef DEVICEOS_HAL_ISECUREELEMENT_H
#define DEVICEOS_HAL_ISECUREELEMENT_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        /**
         * @brief Hardware Cryptographic / Secure Element Interface (e.g., ATECC608, eFuse storage).
         */
        class ISecureElement : public IDriverLifecycle {
        public:
            virtual ~ISecureElement() = default;

            /**
             * @brief Generates a DER-encoded Certificate Signing Request (CSR) on-chip.
             * Private keys must remain secure and unreadable in hardware.
             */
            virtual Result<void> generateCSR(uint8_t* buffer, size_t maxLen, size_t* outLen) = 0;

            /**
             * @brief Retrieves the DER-encoded ECC public key from secure storage.
             */
            virtual Result<void> getPublicKey(uint8_t* buffer, size_t maxLen, size_t* outLen) = 0;

            /**
             * @brief Signs a SHA-256 hash using the target private key slot.
             */
            virtual Result<void> signHash(const uint8_t* hash, size_t hashLen, uint8_t* sigBuffer, size_t maxSigLen, size_t* outSigLen) = 0;

            /**
             * @brief Verifies an ECDSA signature against a SHA-256 hash using a public key.
             */
            virtual Result<bool> verifySignature(const uint8_t* hash, size_t hashLen, const uint8_t* signature, size_t sigLen, const uint8_t* pubKey, size_t pubKeyLen) = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_ISECUREELEMENT_H
