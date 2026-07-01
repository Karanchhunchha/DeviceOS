#include "../sdk/cpp/include/DeviceOS.h"
#include "../runtime/platform/mock/MockStorage.h"
#include "../runtime/platform/mock/MockNetwork.h"
#include "../runtime/platform/mock/MockBluetooth.h"
#include "../runtime/platform/mock/MockSecureElement.h"
#include "../runtime/platform/mock/MockOTA.h"
#include "../runtime/platform/mock/MockPower.h"
#include "../runtime/platform/mock/MockLogging.h"
#include "../runtime/platform/mock/MockConfig.h"
#include "../runtime/include/core/StateSync.h"
#include "../runtime/include/core/OfflineCache.h"
#include "../runtime/include/core/EventTypes.h"
#include <iostream>
#include <assert.h>
#include <string.h>

class SDKSubscriber : public DeviceOS::IEventSubscriber {
public:
    uint32_t count = 0;
    uint16_t lastId = 0;
    uint8_t lastData[128] = {0};

    void onEvent(uint16_t eventId, const uint8_t* payload, size_t length) override {
        count++;
        lastId = eventId;
        if (length > 0 && length <= sizeof(lastData)) {
            memcpy(lastData, payload, length);
        }
    }
};

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "DeviceOS Integrated Boot & State Sync Verification" << std::endl;
    std::cout << "==================================================" << std::endl;

    // Instantiate Mock HAL drivers
    DeviceOS::Platform::MockLogging mockLogging;
    DeviceOS::Platform::MockSecureElement mockCrypto;
    DeviceOS::Platform::MockStorage mockStorage;
    DeviceOS::Platform::MockConfig mockConfig;
    DeviceOS::Platform::MockPower mockPower;
    DeviceOS::Platform::MockNetwork mockNetwork;
    DeviceOS::Platform::MockBluetooth mockBLE;
    DeviceOS::Platform::MockOTA mockOTA;

    // Register all drivers in Service Container
    Device.services().registerService(DeviceOS::ServiceId::LOGGING, &mockLogging, &mockLogging);
    Device.services().registerService(DeviceOS::ServiceId::SECURE_ELEMENT, &mockCrypto, &mockCrypto);
    Device.services().registerService(DeviceOS::ServiceId::STORAGE, &mockStorage, &mockStorage);
    Device.services().registerService(DeviceOS::ServiceId::CONFIG, &mockConfig, &mockConfig);
    Device.services().registerService(DeviceOS::ServiceId::POWER, &mockPower, &mockPower);
    Device.services().registerService(DeviceOS::ServiceId::NETWORK, &mockNetwork, &mockNetwork);
    Device.services().registerService(DeviceOS::ServiceId::BLUETOOTH, &mockBLE, &mockBLE);
    Device.services().registerService(DeviceOS::ServiceId::OTA, &mockOTA, &mockOTA);

    // Mount storage partition
    mockStorage.initialize();
    mockStorage.start();
    mockStorage.mountFilesystem("FLASH", "/spiflash");

    // Initialize core system
    DeviceOS::Result<void> bootRes = Device.begin();
    assert(bootRes.isSuccess());

    // ==========================================
    // TEST 1: Write State Register Parameters
    // ==========================================
    std::cout << "[TEST 1] Setting local state registers..." << std::endl;
    
    DeviceOS::Result<void> r1 = Device.state().setInt("temp_limit", 45);
    DeviceOS::Result<void> r2 = Device.state().setFloat("humidity_target", 62.4f);
    DeviceOS::Result<void> r3 = Device.state().setString("device_name", "ESP32-S3-Sensor");

    assert(r1.isSuccess());
    assert(r2.isSuccess());
    assert(r3.isSuccess());

    std::cout << ">> State registers updated locally." << std::endl;

    // ==========================================
    // TEST 2: Offline Caching & Fallback Sweeps
    // ==========================================
    std::cout << "[TEST 2] Testing offline caching fallback sweeps..." << std::endl;

    // Verify network is currently disconnected
    assert(mockNetwork.getStatus() == DeviceOS::NetworkStatus::DISCONNECTED);

    // Trigger state sync. Since we are offline, this should publish to EventBus,
    // which the OfflineCache catches and saves into the virtual flash.
    DeviceOS::Result<void> syncOfflineRes = Device.state().sync();
    assert(syncOfflineRes.isSuccess());

    // Verify that data was successfully cached in mock flash
    auto* storage = Device.services().get<DeviceOS::Platform::MockStorage>(DeviceOS::ServiceId::STORAGE);
    size_t fileSize = 0;
    uint8_t tempBuf[512];
    storage->readFileMock(tempBuf, sizeof(tempBuf), &fileSize);
    assert(fileSize > 0);

    std::cout << ">> Offline caching verified. File size in flash: " << fileSize << " bytes." << std::endl;

    // ==========================================
    // TEST 3: Online Sync & Direct Delivery
    // ==========================================
    std::cout << "[TEST 3] Testing online sync and direct delivery..." << std::endl;

    // Connect mock network
    DeviceOS::Result<void> connectRes = mockNetwork.connect("Home_AP", "WPA3_Secret");
    assert(connectRes.isSuccess());
    assert(mockNetwork.getStatus() == DeviceOS::NetworkStatus::CONNECTED);

    // Set new dirty values
    Device.state().setInt("temp_limit", 50); // Changed from 45

    // Trigger state sync. Since we are online, it directly processes the update
    DeviceOS::Result<void> syncOnlineRes = Device.state().sync();
    assert(syncOnlineRes.isSuccess());

    std::cout << ">> Online sync completed successfully." << std::endl;

    // ==========================================
    // TEST 4: Reconnection Flush Verification
    // ==========================================
    std::cout << "[TEST 4] Testing auto reconnection flush..." << std::endl;

    // Disconnect, update states, and save to flash
    mockNetwork.disconnect();
    assert(mockNetwork.getStatus() == DeviceOS::NetworkStatus::DISCONNECTED);

    Device.state().setString("status", "ACTIVE");
    Device.state().sync(); // Saves to flash

    SDKSubscriber sdkSub;
    for (uint16_t id = 0x3000; id <= 0x300F; ++id) {
        Device.events().subscribe(id, &sdkSub);
    }

    mockNetwork.connect("Home_AP", "WPA3_Secret");

    uint8_t statePayload[2] = { 
        static_cast<uint8_t>(DeviceOS::RuntimeState::INITIALIZED), 
        static_cast<uint8_t>(DeviceOS::RuntimeState::RUNNING) 
    };
    Device.events().publish(DeviceOS::Events::SYS_STATE_CHANGED, statePayload, 2);

    assert(sdkSub.count > 0);
    assert(sdkSub.lastData[0] == 3); // Type: 3 = String
    assert(strcmp(reinterpret_cast<const char*>(&sdkSub.lastData[1]), "ACTIVE") == 0);

    std::cout << ">> Reconnection auto-flush verified. Flushed event count: " << sdkSub.count << std::endl;

    for (uint16_t id = 0x3000; id <= 0x300F; ++id) {
        Device.events().unsubscribe(id, &sdkSub);
    }

    // ==========================================
    // TEST 5: Cloud Ingestion Ingest Loops
    // ==========================================
    std::cout << "[TEST 5] Testing cloud shadow desired patch ingestion..." << std::endl;

    SDKSubscriber deltaSub;
    Device.events().subscribe(DeviceOS::Events::STATE_SHADOW_DELTA_RECEIVED, &deltaSub);

    // Simulate incoming cloud patch message
    const char* desiredPatch = "{\"temp_limit\":55,\"device_name\":\"ESP32-NewName\"}";
    DeviceOS::Result<void> patchRes = Device.state().onShadowSync(reinterpret_cast<const uint8_t*>(desiredPatch), strlen(desiredPatch));
    assert(patchRes.isSuccess());

    // Verify Event delta triggers
    assert(deltaSub.count == 2);
    assert(strcmp(reinterpret_cast<const char*>(deltaSub.lastData), "device_name") == 0);

    std::cout << ">> Cloud desired patch ingested and applied successfully." << std::endl;

    Device.events().unsubscribe(DeviceOS::Events::STATE_SHADOW_DELTA_RECEIVED, &deltaSub);

    // ==========================================
    // TEST 6: Secure Dual-Partition OTA Updater
    // ==========================================
    std::cout << "[TEST 6] Testing secure dual-partition OTA updater..." << std::endl;

    // Register subscriber for OTA start events
    SDKSubscriber otaSub;
    Device.events().subscribe(DeviceOS::Events::DEVICE_OTA_START, &otaSub);

    // 1. Valid signature path
    DeviceOS::Result<void> ota1 = Device.ota().beginUpdate();
    assert(ota1.isSuccess());

    const uint8_t firmwareChunk[] = "MOCK_FIRMWARE_BINARY_DATA";
    DeviceOS::Result<void> ota2 = Device.ota().writeChunk(firmwareChunk, sizeof(firmwareChunk));
    assert(ota2.isSuccess());

    const uint8_t validSig[] = "VALID_ECC_SIGNATURE_DATA";
    DeviceOS::Result<void> ota3 = Device.ota().verifyAndApply(validSig, sizeof(validSig));
    assert(ota3.isSuccess());
    assert(otaSub.count == 1); // DEVICE_OTA_START was published!

    // 2. Invalid signature security violation check
    DeviceOS::Result<void> ota4 = Device.ota().beginUpdate();
    assert(ota4.isSuccess());
    _ = Device.ota().writeChunk(firmwareChunk, sizeof(firmwareChunk));

    const uint8_t invalidSig[] = "INVALID_ECC_SIGNATURE_DATA";
    DeviceOS::Result<void> ota5 = Device.ota().verifyAndApply(invalidSig, sizeof(invalidSig));
    assert(ota5.isError());
    assert(ota5.getError().code() == DeviceOS::ErrorCode::SECURITY_VIOLATION);

    // 3. Rollback recovery flow
    DeviceOS::Result<void> ota6 = Device.ota().rollback();
    assert(ota6.isSuccess());

    std::cout << ">> Secure Boot OTA updater verified." << std::endl;

    Device.events().unsubscribe(DeviceOS::Events::DEVICE_OTA_START, &otaSub);

    std::cout << "==================================================" << std::endl;
    std::cout << "ALL SYSTEM INTEGRATED VERIFICATIONS PASSED!" << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
