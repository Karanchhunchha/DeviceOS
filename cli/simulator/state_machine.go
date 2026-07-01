package simulator

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"math/rand"
	"net/http"
	"sync"
	"time"
)

type DeviceState string

const (
	StateUnprovisioned  DeviceState = "UNPROVISIONED"
	StateBleAdvertising DeviceState = "BLE_ADVERTISING"
	StateProvisioning   DeviceState = "PROVISIONING"
	StateWifiConnecting DeviceState = "WIFI_CONNECTING"
	StateCloudConnect   DeviceState = "CLOUD_CONNECTING"
	StateShadowSync     DeviceState = "SHADOW_SYNC"
	StateRunning        DeviceState = "RUNNING"
)

type DeviceConfig struct {
	DeviceID      string
	CloudServer   string
	BaseBLEPort   int
	LatencyMaxMs  int
	PacketLossPct int
	PowerCyclePct int
	IDIndex       int // for port offset
}

type DeviceStateMachine struct {
	config    DeviceConfig
	state     DeviceState
	transport ProvisioningTransport
	tenantID  string
	mu        sync.RWMutex
}

func NewDeviceStateMachine(cfg DeviceConfig) *DeviceStateMachine {
	port := cfg.BaseBLEPort + cfg.IDIndex
	return &DeviceStateMachine{
		config:    cfg,
		state:     StateUnprovisioned,
		transport: NewHTTPProvisioningTransport(port),
	}
}

func (d *DeviceStateMachine) CurrentState() DeviceState {
	d.mu.RLock()
	defer d.mu.RUnlock()
	return d.state
}

func (d *DeviceStateMachine) setState(state DeviceState) {
	d.mu.Lock()
	defer d.mu.Unlock()
	d.state = state
	fmt.Printf("[%s] State transition -> %s\n", d.config.DeviceID, state)
}

func (d *DeviceStateMachine) Run(ctx context.Context) {
	d.setState(StateUnprovisioned)

	for {
		select {
		case <-ctx.Done():
			return
		default:
		}

		// Simulate random power cycle
		if d.config.PowerCyclePct > 0 && rand.Intn(100) < d.config.PowerCyclePct {
			fmt.Printf("[%s] SIMULATED POWER CYCLE/REBOOT\n", d.config.DeviceID)
			_ = d.transport.StopAdvertising()
			time.Sleep(1 * time.Second)
			d.setState(StateUnprovisioned)
		}

		switch d.CurrentState() {
		case StateUnprovisioned:
			d.setState(StateBleAdvertising)

		case StateBleAdvertising:
			err := d.transport.StartAdvertising(ctx, d.config.DeviceID)
			if err != nil {
				time.Sleep(1 * time.Second)
				continue
			}
			d.setState(StateProvisioning)

		case StateProvisioning:
			fmt.Printf("[%s] Waiting for BLE provisioning on local port %d...\n", d.config.DeviceID, d.config.BaseBLEPort+d.config.IDIndex)
			
			// Wait for provisioning data or timeout (simulating advertising timeout)
			provCtx, cancel := context.WithTimeout(ctx, 30*time.Second)
			data, err := d.transport.WaitProvisioning(provCtx)
			cancel()

			fmt.Printf("[%s] Provisioning data received. Stopping BLE transport (intentional FSM shutdown).\n", d.config.DeviceID)
			_ = d.transport.StopAdvertising()
			fmt.Printf("[%s] BLE transport stopped. Port %d is now closed.\n", d.config.DeviceID, d.config.BaseBLEPort+d.config.IDIndex)

			if err != nil {
				if err == context.DeadlineExceeded {
					fmt.Printf("[%s] Advertising timeout, restarting...\n", d.config.DeviceID)
					d.setState(StateUnprovisioned)
				}
				continue
			}

			fmt.Printf("[%s] Received Provisioning Data -> SSID: %s, Tenant: %s\n", d.config.DeviceID, data.SSID, data.TenantID)
			d.tenantID = data.TenantID
			d.setState(StateWifiConnecting)

		case StateWifiConnecting:
			// Simulate Wi-Fi connection delay
			time.Sleep(time.Duration(1+rand.Intn(3)) * time.Second)
			d.setState(StateCloudConnect)

		case StateCloudConnect:
			err := d.registerWithCloud()
			if err != nil {
				fmt.Printf("[%s] Cloud connect failed: %v, retrying...\n", d.config.DeviceID, err)
				time.Sleep(2 * time.Second) // Exponential backoff in a real app, linear here for simplicity
				continue
			}
			d.setState(StateShadowSync)

		case StateShadowSync:
			// Simulating initial shadow pull/sync
			time.Sleep(500 * time.Millisecond)
			d.setState(StateRunning)

		case StateRunning:
			d.runTelemetryLoop(ctx)
			// If telemetry loop exits (e.g. Wi-Fi loss), we go back to Cloud Connect
			d.setState(StateCloudConnect)
		}
	}
}

func (d *DeviceStateMachine) registerWithCloud() error {
	regPayload := map[string]string{
		"device_uid": d.config.DeviceID,
		"tenant_id":  d.tenantID,
	}
	regBytes, _ := json.Marshal(regPayload)
	
	// Simulate latency
	if d.config.LatencyMaxMs > 0 {
		time.Sleep(time.Duration(rand.Intn(d.config.LatencyMaxMs)) * time.Millisecond)
	}

	resp, err := http.Post(d.config.CloudServer+"/v1/devices/register", "application/json", bytes.NewBuffer(regBytes))
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode >= 300 {
		return fmt.Errorf("bad status: %d", resp.StatusCode)
	}
	return nil
}

func (d *DeviceStateMachine) runTelemetryLoop(ctx context.Context) {
	for {
		select {
		case <-ctx.Done():
			return
		default:
		}

		// Check random disconnect (simulating Wi-Fi drop)
		if rand.Intn(100) < 5 {
			fmt.Printf("[%s] SIMULATED WI-FI DISCONNECT\n", d.config.DeviceID)
			return // Exit loop to trigger reconnect
		}

		temp := 20.0 + rand.Float64()*10.0
		humidity := 40.0 + rand.Float64()*20.0

		updateBody := map[string]interface{}{
			"state": map[string]interface{}{
				"reported": map[string]interface{}{
					"temperature": temp,
					"humidity":    humidity,
					"status":      "online",
				},
			},
		}
		bodyBytes, _ := json.Marshal(updateBody)

		// Simulate packet loss
		if d.config.PacketLossPct > 0 && rand.Intn(100) < d.config.PacketLossPct {
			fmt.Printf("[%s] SIMULATED PACKET LOSS (dropped telemetry)\n", d.config.DeviceID)
		} else {
			// Simulate Latency
			if d.config.LatencyMaxMs > 0 {
				time.Sleep(time.Duration(rand.Intn(d.config.LatencyMaxMs)) * time.Millisecond)
			}

			url := fmt.Sprintf("%s/v1/devices/%s/shadow", d.config.CloudServer, d.config.DeviceID)
			req, err := http.NewRequest(http.MethodPut, url, bytes.NewBuffer(bodyBytes))
			if err == nil {
				req.Header.Set("Content-Type", "application/json")
				if res, reqErr := http.DefaultClient.Do(req); reqErr == nil {
					res.Body.Close()
					fmt.Printf("[%s] Telemetry synced -> Temp: %.2f, Hum: %.2f\n", d.config.DeviceID, temp, humidity)
				}
			}
		}

		time.Sleep(time.Duration(3+rand.Intn(3)) * time.Second)
	}
}
