#include "../../include/core/ServiceContainer.h"

namespace DeviceOS {
    namespace Core {

        // Enforce static dependency sequencing
        const ServiceId ServiceContainer::s_dependencyOrder[static_cast<size_t>(ServiceId::COUNT)] = {
            ServiceId::LOGGING,
            ServiceId::SECURE_ELEMENT,
            ServiceId::STORAGE,
            ServiceId::CONFIG,
            ServiceId::POWER,
            ServiceId::GPIO,
            ServiceId::BLUETOOTH,
            ServiceId::NETWORK,
            ServiceId::OTA
        };

        ServiceContainer::ServiceContainer() noexcept {
            for (size_t i = 0; i < static_cast<size_t>(ServiceId::COUNT); ++i) {
                m_services[i] = nullptr;
                m_lifecycles[i] = nullptr;
            }
        }

        Result<void> ServiceContainer::registerService(ServiceId id, void* servicePtr, IDriverLifecycle* lifecycle) noexcept {
            if (servicePtr == nullptr) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Service instance pointer cannot be null"));
            }

            size_t index = static_cast<size_t>(id);
            if (index >= static_cast<size_t>(ServiceId::COUNT)) {
                return Result<void>(Error(ErrorCode::INVALID_ARGUMENT, "Invalid Service ID"));
            }

            if (m_services[index] != nullptr) {
                return Result<void>(Error(ErrorCode::STATE_CONFLICT, "Service type is already registered"));
            }

            m_services[index] = servicePtr;
            m_lifecycles[index] = lifecycle;

            return Result<void>();
        }

        void* ServiceContainer::getService(ServiceId id) const noexcept {
            size_t index = static_cast<size_t>(id);
            if (index >= static_cast<size_t>(ServiceId::COUNT)) {
                return nullptr;
            }
            return m_services[index];
        }

        Result<void> ServiceContainer::initializeAll() noexcept {
            for (size_t i = 0; i < static_cast<size_t>(ServiceId::COUNT); ++i) {
                size_t index = static_cast<size_t>(s_dependencyOrder[i]);
                if (m_lifecycles[index] != nullptr) {
                    Result<void> res = m_lifecycles[index]->initialize();
                    if (res.isError()) {
                        return res;
                    }
                }
            }
            return Result<void>();
        }

        Result<void> ServiceContainer::startAll() noexcept {
            for (size_t i = 0; i < static_cast<size_t>(ServiceId::COUNT); ++i) {
                size_t index = static_cast<size_t>(s_dependencyOrder[i]);
                if (m_lifecycles[index] != nullptr) {
                    Result<void> res = m_lifecycles[index]->start();
                    if (res.isError()) {
                        return res;
                    }
                }
            }
            return Result<void>();
        }

        Result<void> ServiceContainer::stopAll() noexcept {
            for (size_t i = static_cast<size_t>(ServiceId::COUNT); i > 0; --i) {
                size_t index = static_cast<size_t>(s_dependencyOrder[i - 1]);
                if (m_lifecycles[index] != nullptr) {
                    Result<void> res = m_lifecycles[index]->stop();
                    if (res.isError()) {
                        return res;
                    }
                }
            }
            return Result<void>();
        }

        Result<void> ServiceContainer::shutdownAll() noexcept {
            for (size_t i = static_cast<size_t>(ServiceId::COUNT); i > 0; --i) {
                size_t index = static_cast<size_t>(s_dependencyOrder[i - 1]);
                if (m_lifecycles[index] != nullptr) {
                    Result<void> res = m_lifecycles[index]->shutdown();
                    if (res.isError()) {
                        return res;
                    }
                }
            }
            return Result<void>();
        }

    } // namespace Core
} // namespace DeviceOS
