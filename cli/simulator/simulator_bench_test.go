package simulator

import (
	"context"
	"testing"
)

// BenchmarkStateTransition measures how fast the state machine loops when errors occur
func BenchmarkStateTransition(b *testing.B) {
	cfg := DeviceConfig{
		DeviceID:      "bench_device",
		CloudServer:   "http://invalid.local",
		BaseBLEPort:   9000,
		IDIndex:       1,
		LatencyMaxMs:  0,
		PacketLossPct: 0,
		PowerCyclePct: 0,
	}

	machine := NewDeviceStateMachine(cfg)
	machine.transport = &MockProvisioningTransport{ShouldFail: true}

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	go machine.Run(ctx)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = machine.CurrentState()
	}
}
