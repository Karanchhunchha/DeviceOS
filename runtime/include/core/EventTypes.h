#ifndef DEVICEOS_CORE_EVENTTYPES_H
#define DEVICEOS_CORE_EVENTTYPES_H

#include <stdint.h>
#include <stddef.h>

namespace DeviceOS {

    /**
     * @brief Priority classes for event ordering and execution queues.
     */
    enum class EventPriority : uint8_t {
        LOW = 0,
        NORMAL,
        HIGH,
        CRITICAL
    };

    /**
     * @brief Event schema representing a system notification.
     * Non-allocating structure designed to carry payload descriptors.
     */
    struct Event {
        uint16_t id;
        EventPriority priority;
        uint32_t timestampMs;
        const uint8_t* payload;
        size_t length;
    };

    /**
     * @brief Filter rule structure for selective event subscription.
     */
    struct EventFilter {
        uint16_t idMask;         // Bitwise mask or exact match
        uint16_t targetId;       // Expected Event ID
        EventPriority minPriority; // Minimum priority threshold

        constexpr EventFilter(uint16_t id = 0, EventPriority priority = EventPriority::LOW)
            : idMask(0xFFFF), targetId(id), minPriority(priority) {}

        constexpr EventFilter(uint16_t mask, uint16_t id, EventPriority priority)
            : idMask(mask), targetId(id), minPriority(priority) {}

        /**
         * @brief Evaluates if an event matches this filter configuration.
         */
        bool matches(const Event& event) const noexcept {
            if ((event.id & idMask) != (targetId & idMask)) {
                return false;
            }
            return static_cast<uint8_t>(event.priority) >= static_cast<uint8_t>(minPriority);
        }
    };

    // System-wide core event identifiers
    namespace Events {
        constexpr uint16_t SYS_STATE_CHANGED  = 0x0001;
        constexpr uint16_t SYS_BOOT_COMPLETE  = 0x0002;
        constexpr uint16_t SYS_ERROR_TRIGGERED = 0x0003;
        
        constexpr uint16_t DRIVER_LIFECYCLE_CHANGED = 0x0010;
        constexpr uint16_t SCHEDULER_OVERRUN_WARN  = 0x0020;
    }

} // namespace DeviceOS

#endif // DEVICEOS_CORE_EVENTTYPES_H
