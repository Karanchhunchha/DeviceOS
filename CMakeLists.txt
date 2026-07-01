cmake_minimum_required(VERSION 3.16)
project(DeviceOS LANGUAGES CXX)

# Require C++20 or newer stable dialect supported by target toolchains
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# DeviceOS Core Static Library
add_library(deviceos_core STATIC
    runtime/src/core/EventBus.cpp
    runtime/src/core/ServiceContainer.cpp
    runtime/src/core/StateMachine.cpp
    runtime/src/core/Scheduler.cpp
    runtime/src/core/OfflineCache.cpp
    runtime/src/core/DeviceCore.cpp
    runtime/src/core/StateSync.cpp
    runtime/src/core/OTAManager.cpp
    runtime/platform/mock/MockStorage.cpp
    runtime/platform/mock/MockNetwork.cpp
    runtime/platform/mock/MockBluetooth.cpp
    runtime/platform/mock/MockSecureElement.cpp
    runtime/platform/mock/MockOTA.cpp
    runtime/platform/mock/MockPower.cpp
    runtime/platform/mock/MockLogging.cpp
    runtime/platform/mock/MockConfig.cpp
)

target_include_directories(deviceos_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cpp/include
)

# Interface HAL configurations
add_library(deviceos_hal INTERFACE)
target_include_directories(deviceos_hal INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cpp/include
)

# Compilation validation test executable
add_executable(compile_test tests/compile_test.cpp)
target_link_libraries(compile_test PRIVATE deviceos_core deviceos_hal)

# Strict embedded compiler flags definition
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(deviceos_core PRIVATE
        -Wall
        -Wextra
        -Werror
        -fno-exceptions
        -fno-rtti
    )
    target_compile_options(compile_test PRIVATE
        -Wall
        -Wextra
        -Werror
        -fno-exceptions
        -fno-rtti
    )
endif()
