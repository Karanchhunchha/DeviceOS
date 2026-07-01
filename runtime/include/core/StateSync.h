#ifndef DEVICEOS_CORE_STATESYNC_H
#define DEVICEOS_CORE_STATESYNC_H

#include "Result.h"
#include "EventBus.h"
#include "ServiceContainer.h"

namespace DeviceOS {

    struct StateRegister {
        char key[32];
        union {
            int32_t iVal;
            float fVal;
            char sVal[64];
        } value;
        uint8_t type; // 0 = None, 1 = Int, 2 = Float, 3 = String
        bool dirty;
        bool occupied;
    };

    namespace Core {

        /**
         * @brief State Synchronization Engine.
         * Manages local parameter registers, tracks dirtiness, and commits changes to the cloud.
         */
        class StateSync {
        public:
            StateSync(EventBus& eventBus, ServiceContainer& container) noexcept;
            ~StateSync() = default;

            // Disable copy
            StateSync(const StateSync&) = delete;
            StateSync& operator=(const StateSync&) = delete;

            Result<void> setInt(const char* key, int32_t val) noexcept;
            Result<void> setFloat(const char* key, float val) noexcept;
            Result<void> setString(const char* key, const char* val) noexcept;

            /**
             * @brief Synchronizes all dirty registers.
             * If online, dispatches HTTP updates. If offline, caches parameters in flash.
             */
            Result<void> sync() noexcept;

            /**
             * @brief Ingests and parses a JSON state patch from the cloud.
             * Updates matching registers, resets dirty flags, and emits notifications.
             */
            Result<void> onShadowPatch(const uint8_t* patchPayload, size_t length) noexcept;

            // Test helper getters
            const StateRegister* getRegisters() const noexcept { return m_registers; }
            size_t getLimit() const noexcept { return REGISTERS_LIMIT; }

        private:
            EventBus& m_eventBus;
            ServiceContainer& m_container;

            static constexpr size_t REGISTERS_LIMIT = 16;
            StateRegister m_registers[REGISTERS_LIMIT];

            StateRegister* findOrAllocate(const char* key) noexcept;
        };

    } // namespace Core
} // namespace DeviceOS

#endif // DEVICEOS_CORE_STATESYNC_H
