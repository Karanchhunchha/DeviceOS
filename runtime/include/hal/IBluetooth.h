#ifndef DEVICEOS_HAL_IBLUETOOTH_H
#define DEVICEOS_HAL_IBLUETOOTH_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        enum class BLEState : uint8_t {
            IDLE = 0,
            ADVERTISING,
            CONNECTED,
            DISCONNECTED
        };

        struct GATTCharacteristic {
            const char* uuid;
            uint16_t properties; // e.g., Bitmask: 0x02 = Read, 0x08 = Write, 0x10 = Notify
            uint8_t* buffer;
            size_t maxLen;
            size_t* currentLen;
            void (*onWriteCallback)(const uint8_t* data, size_t len, void* arg);
            void* callbackArg;
        };

        struct GATTService {
            const char* uuid;
            GATTCharacteristic* characteristics;
            size_t characteristicCount;
        };

        /**
         * @brief BLE / Bluetooth peripheral hardware abstraction layer.
         */
        class IBluetooth : public IDriverLifecycle {
        public:
            virtual ~IBluetooth() = default;

            /**
             * @brief Configures GATT server services database statically.
             */
            virtual Result<void> registerGATTService(const GATTService& service) = 0;

            /**
             * @brief Starts BLE GATT advertising.
             */
            virtual Result<void> startAdvertising(const char* deviceName) = 0;

            /**
             * @brief Stops BLE GATT advertising.
             */
            virtual Result<void> stopAdvertising() = 0;

            /**
             * @brief Sends a GATT notification to connected clients for a target characteristic UUID.
             */
            virtual Result<void> notifyCharacteristic(const char* charUuid, const uint8_t* data, size_t len) = 0;

            /**
             * @brief Retrieves current Bluetooth operational state.
             */
            virtual BLEState getBLEState() const noexcept = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_IBLUETOOTH_H
