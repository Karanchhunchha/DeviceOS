#include "../../sdk/cpp/include/DeviceOS.h"
#include "../../include/core/EventBus.h"
#include "../../include/core/StateMachine.h"
#include "../../include/core/OfflineCache.h"
#include "../../include/core/StateSync.h"
#include "../../include/core/OTAManager.h"
#include <string.h>
#include <new> // Required for placement-new

namespace DeviceOS {

    // Static instances of Core Kernel Engines (Zero Heap allocation)
    static Core::EventBus s_coreEventBus;
    static Core::StateMachine s_stateMachine(s_coreEventBus);

    // Placement-new memory contexts to bypass heap calls
    static uint8_t s_offlineCacheBuf[sizeof(Core::OfflineCache)];
    static uint8_t s_stateSyncBuf[sizeof(Core::StateSync)];
    static uint8_t s_otaManagerBuf[sizeof(Core::OTAManager)];

    static Core::OfflineCache* s_offlineCachePtr = nullptr;
    static Core::StateSync* s_stateSyncPtr = nullptr;
    static Core::OTAManager* s_otaManagerPtr = nullptr;

    // Bridge Adapter to map public SDK subscribers to internal Core subscribers
    class SDKSubscriberBridge : public IEventSubscriber {
    public:
        DeviceOS::IEventSubscriber* sdkSub = nullptr;

        void onEvent(const Event& event) override {
            if (sdkSub != nullptr) {
                sdkSub->onEvent(event.id, event.payload, event.length);
            }
        }
    };

    struct AdapterSlot {
        SDKSubscriberBridge bridge;
        EventSubscriberNode node;
        bool occupied;

        AdapterSlot() : node(&bridge, EventFilter(0)), occupied(false) {}
    };

    static constexpr size_t MAX_SDK_SUBSCRIBERS = 16;
    static AdapterSlot s_adapters[MAX_SDK_SUBSCRIBERS];

    // ==========================================
    // SDK EventBus Wrappers Implementations
    // ==========================================

    Result<void> EventBus::publish(uint16_t eventId, const uint8_t* payload, size_t length) {
        Core::EventBus* coreBus = static_cast<Core::EventBus*>(m_impl);
        if (coreBus == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "Event bus engine not initialized"));
        }

        Event event;
        event.id = eventId;
        event.priority = EventPriority::NORMAL;
        event.timestampMs = 0;
        event.payload = payload;
        event.length = length;

        return coreBus->publishSync(event);
    }

    Result<void> EventBus::subscribe(uint16_t eventId, IEventSubscriber* subscriber) {
        Core::EventBus* coreBus = static_cast<Core::EventBus*>(m_impl);
        if (coreBus == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "Event bus engine not initialized"));
        }
        if (subscriber == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Subscriber cannot be null"));
        }

        int freeIdx = -1;
        for (size_t i = 0; i < MAX_SDK_SUBSCRIBERS; ++i) {
            if (s_adapters[i].occupied && 
                s_adapters[i].bridge.sdkSub == subscriber &&
                s_adapters[i].node.filter.targetId == eventId) 
            {
                return Result<void>(Error(ErrorCode::STATE_CONFLICT, "Subscriber already registered for this event"));
            }
            if (!s_adapters[i].occupied && freeIdx == -1) {
                freeIdx = static_cast<int>(i);
            }
        }

        if (freeIdx == -1) {
            return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "SDK subscriber adapter limits exceeded"));
        }

        size_t idx = static_cast<size_t>(freeIdx);
        s_adapters[idx].bridge.sdkSub = subscriber;
        s_adapters[idx].node.filter = EventFilter(eventId, EventPriority::LOW);
        s_adapters[idx].occupied = true;

        return coreBus->subscribe(s_adapters[idx].node);
    }

    Result<void> EventBus::unsubscribe(uint16_t eventId, IEventSubscriber* subscriber) {
        Core::EventBus* coreBus = static_cast<Core::EventBus*>(m_impl);
        if (coreBus == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "Event bus engine not initialized"));
        }

        for (size_t i = 0; i < MAX_SDK_SUBSCRIBERS; ++i) {
            if (s_adapters[i].occupied && 
                s_adapters[i].bridge.sdkSub == subscriber &&
                s_adapters[i].node.filter.targetId == eventId) 
            {
                Result<void> res = coreBus->unsubscribe(s_adapters[i].node);
                s_adapters[i].occupied = false;
                s_adapters[i].bridge.sdkSub = nullptr;
                return res;
            }
        }

        return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Subscription registration not found"));
    }

    // ==========================================
    // SDK StateManager Wrappers Implementations
    // ==========================================

    Result<void> StateManager::setInt(const char* key, int32_t value) {
        Core::StateSync* coreSync = static_cast<Core::StateSync*>(m_impl);
        if (coreSync == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "StateManager not initialized"));
        }
        return coreSync->setInt(key, value);
    }

    Result<void> StateManager::setFloat(const char* key, float value) {
        Core::StateSync* coreSync = static_cast<Core::StateSync*>(m_impl);
        if (coreSync == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "StateManager not initialized"));
        }
        return coreSync->setFloat(key, value);
    }

    Result<void> StateManager::setString(const char* key, const char* value) {
        Core::StateSync* coreSync = static_cast<Core::StateSync*>(m_impl);
        if (coreSync == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "StateManager not initialized"));
        }
        return coreSync->setString(key, value);
    }

    Result<void> StateManager::sync() {
        Core::StateSync* coreSync = static_cast<Core::StateSync*>(m_impl);
        if (coreSync == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "StateManager not initialized"));
        }
        return coreSync->sync();
    }

    Result<void> StateManager::onShadowSync(const uint8_t* patchPayload, size_t length) {
        Core::StateSync* coreSync = static_cast<Core::StateSync*>(m_impl);
        if (coreSync == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "StateManager not initialized"));
        }
        return coreSync->onShadowPatch(patchPayload, length);
    }

    // ==========================================
    // SDK OTAManager Wrappers Implementations
    // ==========================================

    Result<void> OTAManager::beginUpdate() {
        Core::OTAManager* coreOTA = static_cast<Core::OTAManager*>(m_impl);
        if (coreOTA == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "OTAManager not initialized"));
        }
        return coreOTA->beginUpdate();
    }

    Result<void> OTAManager::writeChunk(const uint8_t* chunk, size_t len) {
        Core::OTAManager* coreOTA = static_cast<Core::OTAManager*>(m_impl);
        if (coreOTA == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "OTAManager not initialized"));
        }
        return coreOTA->writeChunk(chunk, len);
    }

    Result<void> OTAManager::verifyAndApply(const uint8_t* signature, size_t sigLen) {
        Core::OTAManager* coreOTA = static_cast<Core::OTAManager*>(m_impl);
        if (coreOTA == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "OTAManager not initialized"));
        }
        return coreOTA->verifyAndApply(signature, sigLen);
    }

    Result<void> OTAManager::rollback() {
        Core::OTAManager* coreOTA = static_cast<Core::OTAManager*>(m_impl);
        if (coreOTA == nullptr) {
            return Result<void>(Error(ErrorCode::INVALID_STATE, "OTAManager not initialized"));
        }
        return coreOTA->rollback();
    }

    // ==========================================
    // SDK DeviceCore Implementations
    // ==========================================

    DeviceCore::DeviceCore() noexcept {
        m_coreEventBus = &s_coreEventBus;
        m_stateMachine = &s_stateMachine;

        // Initialize structures inside pre-allocated static slots
        s_offlineCachePtr = ::new (s_offlineCacheBuf) Core::OfflineCache(s_coreEventBus, m_serviceContainer);
        s_stateSyncPtr = ::new (s_stateSyncBuf) Core::StateSync(s_coreEventBus, m_serviceContainer);
        s_otaManagerPtr = ::new (s_otaManagerBuf) Core::OTAManager(s_coreEventBus, m_serviceContainer);

        m_offlineCache = s_offlineCachePtr;
        m_stateSync = s_stateSyncPtr;
        m_otaManagerPtr = s_otaManagerPtr;

        m_eventBus.m_impl = &s_coreEventBus;
        m_stateManager.m_impl = s_stateSyncPtr;
        m_otaManager.m_impl = s_otaManagerPtr;
    }

    Result<void> DeviceCore::begin() {
        Result<void> initRes = m_serviceContainer.initializeAll();
        if (initRes.isError()) {
            static_cast<Core::StateMachine*>(m_stateMachine)->transitionTo(RuntimeState::FAULT, 0);
            return initRes;
        }

        Result<void> startRes = m_serviceContainer.startAll();
        if (startRes.isError()) {
            static_cast<Core::StateMachine*>(m_stateMachine)->transitionTo(RuntimeState::FAULT, 0);
            return startRes;
        }

        Result<void> stateRes = static_cast<Core::StateMachine*>(m_stateMachine)->transitionTo(RuntimeState::INITIALIZED, 0);
        return stateRes;
    }

    void DeviceCore::process() {
        static_cast<Core::EventBus*>(m_coreEventBus)->dispatchPending();
    }

} // namespace DeviceOS
