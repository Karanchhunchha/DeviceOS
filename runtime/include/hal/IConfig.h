#ifndef DEVICEOS_HAL_ICONFIG_H
#define DEVICEOS_HAL_ICONFIG_H

#include "../core/ILifecycle.h"

namespace DeviceOS {
    namespace HAL {

        /**
         * @brief Standardized System Configuration Key-Value Store Interface.
         */
        class IConfig : public IDriverLifecycle {
        public:
            virtual ~IConfig() = default;

            /**
             * @brief Saves a signed integer configuration value.
             */
            virtual Result<void> setInt(const char* key, int32_t val) = 0;

            /**
             * @brief Reads a signed integer configuration value, returning a default value if missing.
             */
            virtual Result<int32_t> getInt(const char* key, int32_t defaultVal) const noexcept = 0;

            /**
             * @brief Saves a floating-point configuration value.
             */
            virtual Result<void> setFloat(const char* key, float val) = 0;

            /**
             * @brief Reads a floating-point configuration value, returning a default value if missing.
             */
            virtual Result<float> getFloat(const char* key, float defaultVal) const noexcept = 0;

            /**
             * @brief Saves a string configuration value.
             */
            virtual Result<void> setString(const char* key, const char* val) = 0;

            /**
             * @brief Reads a string configuration value into a caller-allocated char buffer.
             */
            virtual Result<void> getString(const char* key, char* buffer, size_t maxLen) const noexcept = 0;

            /**
             * @brief Forces serialization and commits cached variables to secure flash partitions.
             */
            virtual Result<void> commit() = 0;

            /**
             * @brief Deletes all stored configuration variables.
             */
            virtual Result<void> eraseAll() = 0;
        };

    } // namespace HAL
} // namespace DeviceOS

#endif // DEVICEOS_HAL_ICONFIG_H
