#ifndef DEVICEOS_CORE_STATEMACHINE_H
#define DEVICEOS_CORE_STATEMACHINE_H

#include "Result.h"
#include "EventBus.h"
#include <atomic>

namespace DeviceOS {

    enum class RuntimeState : uint8_t {
        BOOTING = 0,
        INITIALIZED,
        PROVISIONING,
        CONNECTING,
        RUNNING,
        FAULT,
        UPDATING,
        COUNT
    };

    struct TransitionRecord {
        uint32_t timestampMs;
        RuntimeState fromState;
        RuntimeState toState;
    };

    namespace Core {

        /**
         * @brief System Operational State Machine Engine.
         * Enforces valid transition paths, maintains execution histories, and emits change events.
         */
        class StateMachine {
        public:
            explicit StateMachine(EventBus& eventBus) noexcept;
            ~StateMachine() = default;

            // Disable copy
            StateMachine(const StateMachine&) = delete;
            StateMachine& operator=(const StateMachine&) = delete;

            /**
             * @brief Returns current active state.
             */
            RuntimeState getState() const noexcept {
                return m_state.load(std::memory_order_acquire);
            }

            /**
             * @brief Attempts state transition, verifying validation constraints.
             * Emits EVENT_STATE_CHANGED upon successful transition.
             */
            Result<void> transitionTo(RuntimeState newState, uint32_t timestampMs) noexcept;

            /**
             * @brief Retrieves active transition history log.
             * Buffer is maintained as a static circular buffer.
             */
            const TransitionRecord* getHistory() const noexcept {
                return m_history;
            }

            size_t getHistoryHead() const noexcept {
                return m_historyHead;
            }

            size_t getHistoryCount() const noexcept {
                return m_historyCount;
            }

            /**
             * @brief Formats state enum to static string representation.
             */
            static const char* getStateName(RuntimeState state) noexcept;

        private:
            EventBus& m_eventBus;
            std::atomic<RuntimeState> m_state;

            static constexpr size_t HISTORY_SIZE = 8;
            TransitionRecord m_history[HISTORY_SIZE];
            size_t m_historyHead;
            size_t m_historyCount;

            bool isValidTransition(RuntimeState from, RuntimeState to) const noexcept;
        };

    } // namespace Core
} // namespace DeviceOS

#endif // DEVICEOS_CORE_STATEMACHINE_H
