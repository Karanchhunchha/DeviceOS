#ifndef DEVICEOS_CORE_SCHEDULER_H
#define DEVICEOS_CORE_SCHEDULER_H

#include "Result.h"
#include "EventBus.h"

namespace DeviceOS {

    typedef void (*TaskCallback)(void* arg);

    struct ScheduledTask {
        const char* name;
        TaskCallback callback;
        void* arg;
        uint32_t intervalMs;
        uint32_t lastRunMs;
        uint32_t maxDurationUs; // Microseconds execution limit
        bool isPeriodic;
        bool isActive;
    };

    namespace Core {

        /**
         * @brief High-performance cooperative scheduler Engine.
         * Evaluates task execution time, flags overruns, and runs without dynamic allocations.
         */
        class Scheduler {
        public:
            explicit Scheduler(EventBus& eventBus) noexcept;
            ~Scheduler() = default;

            // Disable copy
            Scheduler(const Scheduler&) = delete;
            Scheduler& operator=(const Scheduler&) = delete;

            /**
             * @brief Registers a task. Bounded to static maximum slots.
             */
            Result<void> registerTask(const char* name, 
                                      TaskCallback callback, 
                                      void* arg, 
                                      uint32_t intervalMs, 
                                      uint32_t maxDurationUs, 
                                      bool isPeriodic) noexcept;

            /**
             * @brief Deregisters a task by name.
             */
            Result<void> deregisterTask(const char* name) noexcept;

            /**
             * @brief Periodically checks timer status and runs scheduled tasks.
             */
            void tick(uint32_t currentMs) noexcept;

            /**
             * @brief Injects platform-specific high-resolution timer callback (in microseconds).
             */
            void setMicrosCallback(uint64_t (*callback)()) noexcept {
                m_microsCallback = callback;
            }

        private:
            EventBus& m_eventBus;
            static constexpr size_t MAX_TASKS = 8;
            ScheduledTask m_tasks[MAX_TASKS];
            uint64_t (*m_microsCallback)() = nullptr;
        };

    } // namespace Core
} // namespace DeviceOS

#endif // DEVICEOS_CORE_SCHEDULER_H
