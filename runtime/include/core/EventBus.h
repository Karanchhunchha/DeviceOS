#ifndef DEVICEOS_CORE_EVENTBUS_H
#define DEVICEOS_CORE_EVENTBUS_H

#include "Result.h"
#include "EventTypes.h"
#include <atomic>

namespace DeviceOS {

    class IEventSubscriber {
    public:
        virtual ~IEventSubscriber() = default;
        virtual void onEvent(const Event& event) = 0;
    };

    /**
     * @brief Intrusive linked-list node for event subscribers.
     * The subscriber allocates this node in its own memory scope (e.g. member variable).
     * This avoids all dynamic heap allocation.
     */
    struct EventSubscriberNode {
        IEventSubscriber* subscriber;
        EventFilter filter;
        EventSubscriberNode* next = nullptr;

        EventSubscriberNode(IEventSubscriber* sub, const EventFilter& filt)
            : subscriber(sub), filter(filt), next(nullptr) {}
    };

    namespace Core {

        /**
         * @brief High-performance, Zero-Heap, Hardware-Independent Event Bus Engine.
         * Supports synchronous and asynchronous event dispatching.
         */
        class EventBus {
        public:
            EventBus() : m_head(nullptr), m_queueHead(0), m_queueTail(0), m_arenaOffset(0) {}

            ~EventBus() = default;

            // Disable copy
            EventBus(const EventBus&) = delete;
            EventBus& operator=(const EventBus&) = delete;

            /**
             * @brief Intrusive thread-safe registration of subscriber node.
             */
            Result<void> subscribe(EventSubscriberNode& node) noexcept;

            /**
             * @brief Thread-safe removal of subscriber node.
             */
            Result<void> unsubscribe(EventSubscriberNode& node) noexcept;

            /**
             * @brief Publishes an event synchronously. 
             * Execution runs on the caller's thread context immediately.
             */
            Result<void> publishSync(const Event& event) noexcept;

            /**
             * @brief Publishes an event asynchronously.
             * Payload data is copied into the internal static arena ring-buffer.
             */
            Result<void> publishAsync(const Event& event) noexcept;

            /**
             * @brief Processes and dispatches pending asynchronous events.
             * Must be periodically invoked by the Runtime loop / background task.
             */
            void dispatchPending() noexcept;

            /**
             * @brief Emits a state change event. Helpful helper method.
             */
            Result<void> emitStateChanged(uint8_t oldState, uint8_t newState) noexcept;

        private:
            struct PendingEvent {
                uint16_t id;
                EventPriority priority;
                uint32_t timestampMs;
                size_t arenaOffset;
                size_t length;
            };

            // Hardware-independent atomic spinlock for modifying subscriber lists and async queue
            class SpinLock {
            private:
                std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
            public:
                void lock() noexcept {
                    while (m_flag.test_and_set(std::memory_order_acquire)) {
                        #if defined(__XTENSA__) || defined(__riscv)
                        asm volatile("nop"); // Microcontroller optimization
                        #endif
                    }
                }
                void unlock() noexcept {
                    m_flag.clear(std::memory_order_release);
                }
            };

            SpinLock m_listLock;
            std::atomic<EventSubscriberNode*> m_head;

            // Asynchronous circular ring-buffer queue configuration
            static constexpr size_t QUEUE_SIZE = 16;
            static constexpr size_t ARENA_SIZE = 512;

            SpinLock m_queueLock;
            PendingEvent m_queue[QUEUE_SIZE];
            size_t m_queueHead;
            size_t m_queueTail;
            uint8_t m_arena[ARENA_SIZE];
            size_t m_arenaOffset;

            bool isQueueFull() const noexcept {
                return ((m_queueHead + 1) % QUEUE_SIZE) == m_queueTail;
            }

            bool isQueueEmpty() const noexcept {
                return m_queueHead == m_queueTail;
            }
        };

    } // namespace Core
} // namespace DeviceOS

#endif // DEVICEOS_CORE_EVENTBUS_H
