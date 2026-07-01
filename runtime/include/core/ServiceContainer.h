#ifndef DEVICEOS_CORE_SERVICECONTAINER_H
#define DEVICEOS_CORE_SERVICECONTAINER_H

#include "Result.h"
#include "ILifecycle.h"

namespace DeviceOS {

    /**
     * @brief Unique Identifiers for all core services and drivers.
     */
    enum class ServiceId : uint8_t {
        LOGGING = 0,
        CONFIG,
        STORAGE,
        POWER,
        SECURE_ELEMENT,
        GPIO,
        NETWORK,
        BLUETOOTH,
        OTA,
        COUNT // Keep as final element to track size
    };

    namespace Core {

        /**
         * @brief Inversion of Control (IoC) Service Container.
         * Manages dependencies, prevents duplicates, and enforces topological initialization order.
         */
        class ServiceContainer {
        public:
            ServiceContainer() noexcept;
            ~ServiceContainer() = default;

            // Disable copy
            ServiceContainer(const ServiceContainer&) = delete;
            ServiceContainer& operator=(const ServiceContainer&) = delete;

            /**
             * @brief Registers a service instance, checking for duplicates.
             * Optionally takes a pointer to its Lifecycle interface.
             */
            Result<void> registerService(ServiceId id, void* servicePtr, IDriverLifecycle* lifecycle = nullptr) noexcept;

            /**
             * @brief Resolves a service raw pointer by its ServiceId.
             */
            void* getService(ServiceId id) const noexcept;

            /**
             * @brief Type-safe template helper to resolve services.
             */
            template <typename T>
            T* get(ServiceId id) const noexcept {
                return static_cast<T*>(getService(id));
            }

            /**
             * @brief Standardized topological initialization of all registered services.
             */
            Result<void> initializeAll() noexcept;

            /**
             * @brief Starts all registered service tasks.
             */
            Result<void> startAll() noexcept;

            /**
             * @brief Stops all registered service tasks.
             */
            Result<void> stopAll() noexcept;

            /**
             * @brief Tears down all registered services in reverse dependency order.
             */
            Result<void> shutdownAll() noexcept;

        private:
            void* m_services[static_cast<size_t>(ServiceId::COUNT)];
            IDriverLifecycle* m_lifecycles[static_cast<size_t>(ServiceId::COUNT)];

            // Enforce dependency sequence
            static const ServiceId s_dependencyOrder[static_cast<size_t>(ServiceId::COUNT)];
        };

    } // namespace Core
} // namespace DeviceOS

#endif // DEVICEOS_CORE_SERVICECONTAINER_H
