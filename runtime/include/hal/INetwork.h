#ifndef DEVICEOS_HAL_INETWORK_H
#define DEVICEOS_HAL_INETWORK_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        enum class NetworkStatus : uint8_t {
            DISCONNECTED = 0,
            CONNECTING,
            CONNECTED,
            FAILED
        };

        enum class IPVersion : uint8_t {
            IPv4 = 0,
            IPv6
        };

        struct IPAddress {
            IPVersion version;
            union {
                uint8_t ipv4[4];
                uint8_t ipv6[16];
            } address;
        };

        /**
         * @brief Network interface controller abstraction (e.g., Wi-Fi, Ethernet).
         */
        class INetwork : public IDriverLifecycle {
        public:
            virtual ~INetwork() = default;

            /**
             * @brief Initiates asynchronous connection to a target wireless access point.
             */
            virtual Result<void> connect(const char* ssid, const char* passphrase) = 0;

            /**
             * @brief Disconnects from current network and tears down network interfaces.
             */
            virtual Result<void> disconnect() = 0;

            /**
             * @brief Returns current operational status of connection.
             */
            virtual NetworkStatus getStatus() const noexcept = 0;

            /**
             * @brief Retrieves RSSI signal strength metrics in dBm.
             */
            virtual Result<int16_t> getRSSI() const noexcept = 0;

            /**
             * @brief Retrieves the active assigned IP Address.
             */
            virtual Result<IPAddress> getIPAddress() const noexcept = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_INETWORK_H
