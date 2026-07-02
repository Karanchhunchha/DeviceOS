package simulator

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"sync"
)

// ProvisioningData represents the Wi-Fi credentials and tenant sent during BLE provisioning
type ProvisioningData struct {
	SSID     string `json:"ssid"`
	Password string `json:"password"`
	TenantID string `json:"tenant_id"`
}

// ProvisioningTransport is a transport-agnostic interface for the BLE Provisioning phase.
// In the simulator, this will be implemented via an HTTP server listening on a local port.
// On real hardware, this would be an ESP-IDF BLE GATT server.
type ProvisioningTransport interface {
	StartAdvertising(ctx context.Context, deviceID string) error
	StopAdvertising() error
	WaitProvisioning(ctx context.Context) (ProvisioningData, error)
}

// HTTPProvisioningTransport simulates a BLE server using a local HTTP port.
type HTTPProvisioningTransport struct {
	port       int
	server     *http.Server
	dataChan   chan ProvisioningData
	mu         sync.Mutex
	isAdvs     bool
}

func NewHTTPProvisioningTransport(port int) *HTTPProvisioningTransport {
	return &HTTPProvisioningTransport{
		port:     port,
		dataChan: make(chan ProvisioningData, 1),
	}
}

func (t *HTTPProvisioningTransport) StartAdvertising(ctx context.Context, deviceID string) error {
	fmt.Printf("[BLE_TRANSPORT] StartAdvertising: binding HTTP server on port %d (simulates BLE GATT)\n", t.port)
	t.mu.Lock()
	defer t.mu.Unlock()

	if t.isAdvs {
		return nil
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/provision", func(w http.ResponseWriter, r *http.Request) {
		fmt.Printf("\n[SIM_HTTP] Incoming Request: %s %s\n", r.Method, r.URL.String())
		fmt.Printf("[SIM_HTTP] Headers: %v\n", r.Header)

		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if r.Method == http.MethodOptions {
			fmt.Printf("[SIM_HTTP] Handled OPTIONS preflight request. Returning 200 OK.\n")
			w.WriteHeader(http.StatusOK)
			return
		}

		if r.Method != http.MethodPost {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		body, err := io.ReadAll(r.Body)
		if err != nil {
			http.Error(w, "Failed to read body", http.StatusBadRequest)
			return
		}

		var data ProvisioningData
		if err := json.Unmarshal(body, &data); err != nil {
			http.Error(w, "Invalid JSON", http.StatusBadRequest)
			return
		}

		// Send data to the state machine
		select {
		case t.dataChan <- data:
			w.WriteHeader(http.StatusOK)
			fmt.Fprintf(w, `{"status":"provisioned"}`)
		default:
			http.Error(w, "Already provisioned or busy", http.StatusConflict)
		}
	})

	t.server = &http.Server{
		Addr:    fmt.Sprintf("0.0.0.0:%d", t.port),
		Handler: mux,
	}

	go func() {
		_ = t.server.ListenAndServe()
	}()

	t.isAdvs = true
	return nil
}

func (t *HTTPProvisioningTransport) StopAdvertising() error {
	t.mu.Lock()
	defer t.mu.Unlock()

	if !t.isAdvs || t.server == nil {
		return nil
	}

	fmt.Printf("[BLE_TRANSPORT] StopAdvertising: closing HTTP server on port %d — BLE advertising OFF\n", t.port)
	err := t.server.Close()
	t.isAdvs = false
	return err
}

func (t *HTTPProvisioningTransport) WaitProvisioning(ctx context.Context) (ProvisioningData, error) {
	select {
	case data := <-t.dataChan:
		return data, nil
	case <-ctx.Done():
		return ProvisioningData{}, ctx.Err()
	}
}
