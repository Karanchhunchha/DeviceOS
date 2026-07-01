#ifndef DEVICEOS_CORE_RESULT_H
#define DEVICEOS_CORE_RESULT_H

#include <stdint.h>
#include <new>

namespace DeviceOS {

    /**
     * @brief System-wide error codes.
     */
    enum class ErrorCode : uint32_t {
        OK = 0,
        INVALID_ARGUMENT,
        TIMEOUT,
        OUT_OF_MEMORY,
        BUS_ERROR,
        STATE_CONFLICT,
        DRIVER_NOT_FOUND,
        AUTH_FAILURE,
        CONNECTION_LOST,
        HARDWARE_FAILURE,
        NOT_SUPPORTED,
        INVALID_STATE
    };

    /**
     * @brief Encapsulates a code and descriptive static message for failure states.
     */
    struct Error {
        ErrorCode code;
        const char* message; // Must point to a static string literal

        constexpr Error(ErrorCode c, const char* msg) : code(c), message(msg) {}
    };

    /**
     * @brief Result class containing either a success value T or an Error.
     * Designed for embedded environments: no exceptions, no heap allocations.
     */
    template <typename T>
    class Result {
    public:
        // Constructors
        Result(const T& val) : m_isSuccess(true) {
            new (&m_storage.value) T(val);
        }

        Result(T&& val) : m_isSuccess(true) {
            new (&m_storage.value) T(std::move(val));
        }

        Result(const Error& err) : m_isSuccess(false) {
            new (&m_storage.error) Error(err);
        }

        // Destructor
        ~Result() {
            if (m_isSuccess) {
                m_storage.value.~T();
            } else {
                m_storage.error.~Error();
            }
        }

        // Disable copy/move constructors for simplicity unless explicitly needed
        Result(const Result& other) : m_isSuccess(other.m_isSuccess) {
            if (m_isSuccess) {
                new (&m_storage.value) T(other.m_storage.value);
            } else {
                new (&m_storage.error) Error(other.m_storage.error);
            }
        }

        Result(Result&& other) noexcept : m_isSuccess(other.m_isSuccess) {
            if (m_isSuccess) {
                new (&m_storage.value) T(std::move(other.m_storage.value));
            } else {
                new (&m_storage.error) Error(other.m_storage.error);
            }
        }

        // Status queries
        bool isSuccess() const noexcept { return m_isSuccess; }
        bool isError() const noexcept { return !m_isSuccess; }

        explicit operator bool() const noexcept { return m_isSuccess; }

        // Value accessors
        T& value() & {
            return m_storage.value;
        }

        const T& value() const & {
            return m_storage.value;
        }

        T&& value() && {
            return std::move(m_storage.value);
        }

        // Error accessor
        const Error& error() const noexcept {
            return m_storage.error;
        }

    private:
        bool m_isSuccess;
        union Storage {
            T value;
            Error error;

            Storage() {}
            ~Storage() {}
        } m_storage;
    };

    /**
     * @brief Specialization of Result for functions that return no value but can fail.
     */
    template <>
    class Result<void> {
    public:
        Result() : m_isSuccess(true), m_error(ErrorCode::OK, "Success") {}
        Result(const Error& err) : m_isSuccess(false), m_error(err) {}

        bool isSuccess() const noexcept { return m_isSuccess; }
        bool isError() const noexcept { return !m_isSuccess; }

        explicit operator bool() const noexcept { return m_isSuccess; }

        const Error& error() const noexcept {
            return m_error;
        }

    private:
        bool m_isSuccess;
        Error m_error;
    };

} // namespace DeviceOS

#endif // DEVICEOS_CORE_RESULT_H
