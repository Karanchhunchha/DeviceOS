#include "../../include/core/EventBus.h"
#include <string.h>

namespace DeviceOS {
    namespace Core {

        Result<void> EventBus::subscribe(EventSubscriberNode& node) noexcept {
            if (node.subscriber == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Subscriber cannot be null"));
            }

            m_listLock.lock();
            
            // Prevent duplicate subscription of the same node
            EventSubscriberNode* curr = m_head.load(std::memory_order_relaxed);
            while (curr != nullptr) {
                if (curr == &node) {
                    m_listLock.unlock();
                    return Result<void>(Error(ErrorCode::STATE_CONFLICT, "Node already subscribed"));
                }
                curr = curr->next;
            }

            node.next = m_head.load(std::memory_order_relaxed);
            m_head.store(&node, std::memory_order_release);
            
            m_listLock.unlock();
            return Result<void>();
        }

        Result<void> EventBus::unsubscribe(EventSubscriberNode& node) noexcept {
            m_listLock.lock();

            EventSubscriberNode* curr = m_head.load(std::memory_order_relaxed);
            EventSubscriberNode* prev = nullptr;

            while (curr != nullptr) {
                if (curr == &node) {
                    if (prev == nullptr) {
                        m_head.store(curr->next, std::memory_order_release);
                    } else {
                        prev->next = curr->next;
                    }
                    curr->next = nullptr;
                    m_listLock.unlock();
                    return Result<void>();
                }
                prev = curr;
                curr = curr->next;
            }

            m_listLock.unlock();
            return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Node not found in subscriber list"));
        }

        Result<void> EventBus::publishSync(const Event& event) noexcept {
            EventSubscriberNode* curr = m_head.load(std::memory_order_acquire);
            
            while (curr != nullptr) {
                if (curr->filter.matches(event)) {
                    curr->subscriber->onEvent(event);
                }
                curr = curr->next;
            }
            
            return Result<void>();
        }

        Result<void> EventBus::publishAsync(const Event& event) noexcept {
            if (event.length > ARENA_SIZE) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Payload exceeds total arena buffer size"));
            }

            m_queueLock.lock();

            if (isQueueFull()) {
                m_queueLock.unlock();
                return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Async event queue is full"));
            }

            size_t writeOffset = m_arenaOffset;
            if (writeOffset + event.length > ARENA_SIZE) {
                writeOffset = 0;
            }

            size_t tempTail = m_queueTail;
            while (tempTail != m_queueHead) {
                const PendingEvent& activeEvent = m_queue[tempTail];
                if (activeEvent.length > 0) {
                    size_t activeStart = activeEvent.arenaOffset;
                    size_t activeEnd = activeStart + activeEvent.length;
                    
                    size_t writeStart = writeOffset;
                    size_t writeEnd = writeStart + event.length;

                    if (!(writeEnd <= activeStart || writeStart >= activeEnd)) {
                        m_queueLock.unlock();
                        return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Arena buffer collision: payload space is locked"));
                    }
                }
                tempTail = (tempTail + 1) % QUEUE_SIZE;
            }

            if (event.length > 0 && event.payload != nullptr) {
                memcpy(&m_arena[writeOffset], event.payload, event.length);
            }

            PendingEvent& pending = m_queue[m_queueHead];
            pending.id = event.id;
            pending.priority = event.priority;
            pending.timestampMs = event.timestampMs;
            pending.arenaOffset = writeOffset;
            pending.length = event.length;

            m_queueHead = (m_queueHead + 1) % QUEUE_SIZE;
            m_arenaOffset = (writeOffset + event.length) % ARENA_SIZE;

            m_queueLock.unlock();
            return Result<void>();
        }

        void EventBus::dispatchPending() noexcept {
            while (true) {
                m_queueLock.lock();
                if (isQueueEmpty()) {
                    m_queueLock.unlock();
                    break;
                }

                size_t tail = m_queueTail;
                const PendingEvent& pending = m_queue[tail];

                Event event;
                event.id = pending.id;
                event.priority = pending.priority;
                event.timestampMs = pending.timestampMs;
                event.payload = (pending.length > 0) ? &m_arena[pending.arenaOffset] : nullptr;
                event.length = pending.length;

                m_queueLock.unlock();

                publishSync(event);

                m_queueLock.lock();
                m_queueTail = (m_queueTail + 1) % QUEUE_SIZE;
                m_queueLock.unlock();
            }
        }

        Result<void> EventBus::emitStateChanged(uint8_t oldState, uint8_t newState) noexcept {
            uint8_t payload[2] = { oldState, newState };
            Event event;
            event.id = Events::SYS_STATE_CHANGED;
            event.priority = EventPriority::HIGH;
            event.timestampMs = 0;
            event.payload = payload;
            event.length = 2;

            return publishSync(event);
        }

    } // namespace Core
} // namespace DeviceOS
