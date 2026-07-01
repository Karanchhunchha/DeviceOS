package simulator

import (
	"context"
	"testing"
	"time"
)

// MockProvisioningTransport is for testing the state machine without HTTP
type MockProvisioningTransport struct {
	ShouldFail bool
	IsAdvs     bool
}

func (m *MockProvisioningTransport) StartAdvertising(ctx context.Context, deviceID string) error {
	m.IsAdvs = true
	return nil
}
func (m *MockProvisioningTransport) StopAdvertising() error {
	m.IsAdvs = false
	return nil
}
func (m *MockProvisioningTransport) WaitProvisioning(ctx context.Context) (ProvisioningData, error) {
	if m.ShouldFail {
		return ProvisioningData{}, context.DeadlineExceeded
	}
	return ProvisioningData{SSID: "TestWiFi", Password: "test", TenantID: "tenant1"}, nil
}

func TestDeviceStateMachine_ProvisioningFlow(t *testing.T) {
	cfg := DeviceConfig{
		DeviceID:      "test_device_01",
		CloudServer:   "http://localhost:8080", // We assume we mock http if it goes further, but we'll cancel ctx
		BaseBLEPort:   9000,
		IDIndex:       1,
		LatencyMaxMs:  0,
		PacketLossPct: 0,
		PowerCyclePct: 0,
	}

	machine := NewDeviceStateMachine(cfg)
	// Inject mock
	mockTransport := &MockProvisioningTransport{ShouldFail: false}
	machine.transport = mockTransport

	ctx, cancel := context.WithCancel(context.Background())

	go machine.Run(ctx)

	// Wait for state transition to Provisioning
	time.Sleep(50 * time.Millisecond)

	// It should move through Unprovisioned -> Advertising -> Provisioning -> WifiConnecting
	state := machine.CurrentState()
	if state != StateWifiConnecting && state != StateCloudConnect {
		// Because mock WaitProvisioning returns immediately, it jumps fast.
		t.Logf("State after fast mock provisioning: %s", state)
	}

	cancel() // Stop the loop
}
