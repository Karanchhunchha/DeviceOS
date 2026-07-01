#include "../../include/core/StateMachine.h"

namespace DeviceOS {
    namespace Core {

        StateMachine::StateMachine(EventBus& eventBus) noexcept
            : m_eventBus(eventBus), 
              m_state(RuntimeState::BOOTING),
              m_historyHead(0), 
              m_historyCount(0) 
        {
            for (size_t i = 0; i < HISTORY_SIZE; ++i) {
                m_history[i].timestampMs = 0;
                m_history[i].fromState = RuntimeState::BOOTING;
                m_history[i].toState = RuntimeState::BOOTING;
            }
        }

        Result<void> StateMachine::transitionTo(RuntimeState newState, uint32_t timestampMs) noexcept {
            RuntimeState expected = m_state.load(std::memory_order_acquire);

            while (true) {
                if (!isValidTransition(expected, newState)) {
                    return Result<void>(Error(ErrorCode::INVALID_STATE, "State transition violation: path blocked by rule matrix"));
                }

                if (m_state.compare_exchange_weak(expected, newState, std::memory_order_acq_rel, std::memory_order_acquire)) {
                    break;
                }
            }

            size_t head = m_historyHead;
            m_history[head].timestampMs = timestampMs;
            m_history[head].fromState = expected;
            m_history[head].toState = newState;

            m_historyHead = (head + 1) % HISTORY_SIZE;
            if (m_historyCount < HISTORY_SIZE) {
                m_historyCount++;
            }

            m_eventBus.emitStateChanged(static_cast<uint8_t>(expected), static_cast<uint8_t>(newState));

            return Result<void>();
        }

        bool StateMachine::isValidTransition(RuntimeState from, RuntimeState to) const noexcept {
            if (from == to) return true;

            switch (from) {
                case RuntimeState::BOOTING:
                    return (to == RuntimeState::INITIALIZED || to == RuntimeState::FAULT);

                case RuntimeState::INITIALIZED:
                    return (to == RuntimeState::PROVISIONING || to == RuntimeState::CONNECTING || to == RuntimeState::FAULT);

                case RuntimeState::PROVISIONING:
                    return (to == RuntimeState::CONNECTING || to == RuntimeState::BOOTING || to == RuntimeState::FAULT);

                case RuntimeState::CONNECTING:
                    return (to == RuntimeState::RUNNING || to == RuntimeState::PROVISIONING || to == RuntimeState::INITIALIZED || to == RuntimeState::FAULT);

                case RuntimeState::RUNNING:
                    return (to == RuntimeState::CONNECTING || to == RuntimeState::UPDATING || to == RuntimeState::FAULT);

                case RuntimeState::FAULT:
                    return (to == RuntimeState::BOOTING);

                case RuntimeState::UPDATING:
                    return (to == RuntimeState::BOOTING || to == RuntimeState::FAULT);

                default:
                    return false;
            }
        }

        const char* StateMachine::getStateName(RuntimeState state) noexcept {
            switch (state) {
                case RuntimeState::BOOTING:      return "BOOTING";
                case RuntimeState::INITIALIZED:  return "INITIALIZED";
                case RuntimeState::PROVISIONING: return "PROVISIONING";
                case RuntimeState::CONNECTING:   return "CONNECTING";
                case RuntimeState::RUNNING:      return "RUNNING";
                case RuntimeState::FAULT:        return "FAULT";
                case RuntimeState::UPDATING:     return "UPDATING";
                default:                         return "UNKNOWN";
            }
        }

    } // namespace Core
} // namespace DeviceOS
