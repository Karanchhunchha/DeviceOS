package simulator

import (
	"bytes"
	"context"
	"encoding/json"
	"net/http"
	"testing"
	"time"
)

func TestProvisioningCORSAndFlow(t *testing.T) {
	port := 9099
	transport := NewHTTPProvisioningTransport(port)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Start advertising
	err := transport.StartAdvertising(ctx, "test_dev_cors")
	if err != nil {
		t.Fatalf("Failed to start advertising: %v", err)
	}
	defer transport.StopAdvertising()

	// Give server a moment to bind
	time.Sleep(100 * time.Millisecond)

	url := "http://localhost:9099/provision"

	// 1. Test OPTIONS request (CORS preflight)
	reqOpts, _ := http.NewRequest(http.MethodOptions, url, nil)
	respOpts, err := http.DefaultClient.Do(reqOpts)
	if err != nil {
		t.Fatalf("OPTIONS request failed: %v", err)
	}
	defer respOpts.Body.Close()

	if respOpts.StatusCode != http.StatusOK {
		t.Errorf("Expected 200 OK for OPTIONS, got %d", respOpts.StatusCode)
	}
	if respOpts.Header.Get("Access-Control-Allow-Origin") != "*" {
		t.Errorf("Missing or invalid Access-Control-Allow-Origin header")
	}
	if respOpts.Header.Get("Access-Control-Allow-Methods") != "POST, OPTIONS" {
		t.Errorf("Missing or invalid Access-Control-Allow-Methods header")
	}

	// 2. Test POST request
	data := ProvisioningData{
		SSID:     "TestSSID",
		Password: "TestPassword",
		TenantID: "tenant_test",
	}
	body, _ := json.Marshal(data)
	reqPost, _ := http.NewRequest(http.MethodPost, url, bytes.NewBuffer(body))
	reqPost.Header.Set("Content-Type", "application/json")

	respPost, err := http.DefaultClient.Do(reqPost)
	if err != nil {
		t.Fatalf("POST request failed: %v", err)
	}
	defer respPost.Body.Close()

	if respPost.StatusCode != http.StatusOK {
		t.Errorf("Expected 200 OK for POST, got %d", respPost.StatusCode)
	}

	// 3. Verify WaitProvisioning receives the data
	ctxWait, cancelWait := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancelWait()
	
	received, err := transport.WaitProvisioning(ctxWait)
	if err != nil {
		t.Fatalf("Failed to wait for provisioning data: %v", err)
	}
	
	if received.SSID != data.SSID || received.TenantID != data.TenantID {
		t.Errorf("Received data mismatch. Got %v, expected %v", received, data)
	}
}
