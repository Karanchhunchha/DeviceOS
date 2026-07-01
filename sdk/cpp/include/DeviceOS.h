#ifndef DEVICEOS_SDK_ENTRY_H
#define DEVICEOS_SDK_ENTRY_H

#include "../../../runtime/include/core/Result.h"
#include "../../../runtime/include/core/ILifecycle.h"
#include "../../../runtime/include/core/ServiceContainer.h"

namespace DeviceOS {

    class IEventSubscriber {
    public:
        virtual ~IEventSubscriber() = default;
        virtual void onEvent(uint16_t eventId, const uint8_t* payload, size_t length) = 0;
    };

    /**
     * @brief High-performance lock-free Event Bus.
     * Mediates telemetry broadcasts and module updates dynamically.
     */
    class EventBus {
    private:
        friend class DeviceCore;
        void* m_impl; // Pimpl: pointer to Core::EventBus instance
    public:
        EventBus() : m_impl(nullptr) {}
        Result<void> publish(uint16_t eventId, const uint8_t* payload, size_t length);
        Result<void> subscribe(uint16_t eventId, IEventSubscriber* subscriber);
        Result<void> unsubscribe(uint16_t eventId, IEventSubscriber* subscriber);
    };

    /**
     * @brief Telemetry State Synchronization engine.
     * Manages device shadows and coordinates reported/desired configurations.
     */
    class StateManager {
    private:
        friend class DeviceCore;
        void* m_impl; // Pimpl: pointer to Core::StateSync
    public:
        StateManager() : m_impl(nullptr) {}
        Result<void> setInt(const char* key, int32_t value);
        Result<void> setFloat(const char* key, float value);
        Result<void> setString(const char* key, const char* value);
        
        Result<void> sync(); // Triggers shadow delta publishing
        Result<void> onShadowSync(const uint8_t* patchPayload, size_t length); // Ingests cloud desired updates
    };

    /**
     * @brief Secure Dual-Partition Over-the-Air (OTA) Update Manager.
     */
    class OTAManager {
    private:
        friend class DeviceCore;
        void* m_impl; // Pimpl: pointer to Core::OTAManager
    public:
        OTAManager() : m_impl(nullptr) {}
        Result<void> beginUpdate();
        Result<void> writeChunk(const uint8_t* chunk, size_t len);
        Result<void> verifyAndApply(const uint8_t* signature, size_t sigLen);
        Result<void> rollback();
    };

    /**
     * @brief The DeviceOS core platform engine.
     * Manages system loops, watches timers, and coordinates driver lifecycles.
     */
    class DeviceCore {
    private:
        EventBus m_eventBus;
        StateManager m_stateManager;
        OTAManager m_otaManager;
        Core::ServiceContainer m_serviceContainer;
        
        void* m_coreEventBus;  // Core::EventBus reference
        void* m_stateMachine;  // Core::StateMachine reference
        void* m_offlineCache;  // Core::OfflineCache reference
        void* m_stateSync;     // Core::StateSync reference
        void* m_otaManagerPtr; // Core::OTAManager reference

    public:
        DeviceCore() noexcept;
        ~DeviceCore() = default;

        Result<void> begin();
        void process();

        EventBus& events() noexcept { return m_eventBus; }
        StateManager& state() noexcept { return m_stateManager; }
        OTAManager& ota() noexcept { return m_otaManager; }
        Core::ServiceContainer& services() noexcept { return m_serviceContainer; }
    };

} // namespace DeviceOS

/**
 * @brief Global singleton instance for application logic code.
 */
extern DeviceOS::DeviceCore Device;

#endif // DEVICEOS_SDK_ENTRY_H
