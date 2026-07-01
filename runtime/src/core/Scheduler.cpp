#include "../../include/core/Scheduler.h"
#include <string.h>

namespace DeviceOS {
    namespace Core {

        Scheduler::Scheduler(EventBus& eventBus) noexcept
            : m_eventBus(eventBus), m_microsCallback(nullptr) 
        {
            for (size_t i = 0; i < MAX_TASKS; ++i) {
                m_tasks[i].name = nullptr;
                m_tasks[i].callback = nullptr;
                m_tasks[i].arg = nullptr;
                m_tasks[i].intervalMs = 0;
                m_tasks[i].lastRunMs = 0;
                m_tasks[i].maxDurationUs = 0;
                m_tasks[i].isPeriodic = false;
                m_tasks[i].isActive = false;
            }
        }

        Result<void> Scheduler::registerTask(const char* name, 
                                              TaskCallback callback, 
                                              void* arg, 
                                              uint32_t intervalMs, 
                                              uint32_t maxDurationUs, 
                                              bool isPeriodic) noexcept 
        {
            if (callback == nullptr || name == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Task name and callback cannot be null"));
            }

            for (size_t i = 0; i < MAX_TASKS; ++i) {
                if (m_tasks[i].isActive && strcmp(m_tasks[i].name, name) == 0) {
                    return Result<void>(Error(ErrorCode::STATE_CONFLICT, "Task with this name is already registered"));
                }
            }

            for (size_t i = 0; i < MAX_TASKS; ++i) {
                if (!m_tasks[i].isActive) {
                    m_tasks[i].name = name;
                    m_tasks[i].callback = callback;
                    m_tasks[i].arg = arg;
                    m_tasks[i].intervalMs = intervalMs;
                    m_tasks[i].lastRunMs = 0;
                    m_tasks[i].maxDurationUs = maxDurationUs;
                    m_tasks[i].isPeriodic = isPeriodic;
                    m_tasks[i].isActive = true;
                    return Result<void>();
                }
            }

            return Result<void>(Error(ErrorCode::OUT_OF_MEMORY, "Cooperative scheduler task slots are full"));
        }

        Result<void> Scheduler::deregisterTask(const char* name) noexcept {
            if (name == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Task name cannot be null"));
            }

            for (size_t i = 0; i < MAX_TASKS; ++i) {
                if (m_tasks[i].isActive && strcmp(m_tasks[i].name, name) == 0) {
                    m_tasks[i].isActive = false;
                    m_tasks[i].name = nullptr;
                    m_tasks[i].callback = nullptr;
                    m_tasks[i].arg = nullptr;
                    return Result<void>();
                }
            }

            return Result<void>(Error(ErrorCode::DRIVER_NOT_FOUND, "Task name not found in scheduler registry"));
        }

        void Scheduler::tick(uint32_t currentMs) noexcept {
            for (size_t i = 0; i < MAX_TASKS; ++i) {
                ScheduledTask& task = m_tasks[i];
                if (!task.isActive) {
                    continue;
                }

                if (currentMs - task.lastRunMs >= task.intervalMs) {
                    uint64_t startUs = 0;
                    if (m_microsCallback != nullptr) {
                        startUs = m_microsCallback();
                    }

                    task.callback(task.arg);

                    if (m_microsCallback != nullptr) {
                        uint64_t endUs = m_microsCallback();
                        if (endUs >= startUs) {
                            uint64_t duration = endUs - startUs;
                            if (duration > task.maxDurationUs) {
                                Event event;
                                event.id = Events::SCHEDULER_OVERRUN_WARN;
                                event.priority = EventPriority::HIGH;
                                event.timestampMs = currentMs;
                                event.payload = reinterpret_cast<const uint8_t*>(task.name);
                                event.length = strlen(task.name);

                                m_eventBus.publishSync(event);
                            }
                        }
                    }

                    if (task.isPeriodic) {
                        task.lastRunMs = currentMs;
                    } else {
                        task.isActive = false;
                    }
                }
            }
        }

    } // namespace Core
} // namespace DeviceOS
