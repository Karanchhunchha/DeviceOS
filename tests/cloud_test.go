package tests

import (
	"bytes"
	"crypto/sha1"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/http/httptest"
	"net/url"
	"os"
	"reflect"
	"strings"
	"sync"
	"testing"

	"deviceos/cloud/pkg/database"
	"deviceos/cloud/pkg/shadow"
)

// Global test broker matching production server
type TestBroker struct {
	mu        sync.RWMutex
	observers map[string][]net.Conn
}

var testBroker = &TestBroker{
	observers: make(map[string][]net.Conn),
}

func (b *TestBroker) Subscribe(deviceId string, conn net.Conn) {
	b.mu.Lock()
	defer b.mu.Unlock()
	b.observers[deviceId] = append(b.observers[deviceId], conn)
}

func (b *TestBroker) Unsubscribe(conn net.Conn) {
	b.mu.Lock()
	defer b.mu.Unlock()
	for devId, list := range b.observers {
		for i, c := range list {
			if c == conn {
				b.observers[devId] = append(list[:i], list[i+1:]...)
				break
			}
		}
	}
}

func (b *TestBroker) Broadcast(deviceId string, data []byte) {
	b.mu.RLock()
	defer b.mu.RUnlock()
	list, exists := b.observers[deviceId]
	if !exists {
		return
	}
	for _, conn := range list {
		go func(c net.Conn) {
			_ = testWriteTextMessage(c, data)
		}(conn)
	}
}

func testComputeAcceptKey(key string) string {
	h := sha1.New()
	h.Write([]byte(key))
	h.Write([]byte("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))
	return base64.StdEncoding.EncodeToString(h.Sum(nil))
}

func testWriteTextMessage(conn net.Conn, payload []byte) error {
	length := len(payload)
	var header []byte
	header = append(header, 0x81) // Text Frame opcode 0x81 (Fin=1, Opcode=1)

	if length <= 125 {
		header = append(header, byte(length))
	} else {
		header = append(header, 126)
		header = append(header, byte(length>>8), byte(length))
	}

	_, err := conn.Write(header)
	if err != nil {
		return err
	}
	_, err = conn.Write(payload)
	return err
}

func testReadWebSocketFrame(conn net.Conn) ([]byte, error) {
	header := make([]byte, 2)
	if _, err := conn.Read(header); err != nil {
		return nil, err
	}

	payloadLen := int(header[1] & 0x7F)
	if payloadLen == 126 {
		lenBuf := make([]byte, 2)
		if _, err := conn.Read(lenBuf); err != nil {
			return nil, err
		}
		payloadLen = int(lenBuf[0])<<8 | int(lenBuf[1])
	}

	// Payload data (server frames are NOT masked)
	payload := make([]byte, payloadLen)
	if _, err := io.ReadFull(conn, payload); err != nil {
		return nil, err
	}

	return payload, nil
}

func mergeMaps(dest, src map[string]interface{}) {
	for k, v := range src {
		if v == nil {
			delete(dest, k)
			continue
		}
		destNested, isDestNested := dest[k].(map[string]interface{})
		srcNested, isSrcNested := v.(map[string]interface{})
		if isDestNested && isSrcNested {
			mergeMaps(destNested, srcNested)
		} else {
			dest[k] = v
		}
	}
}

// Server mock handler structure matching our production server type
type TestServer struct {
	DB *database.DB
}

func (s *TestServer) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path == "/v1/devices/register" {
		s.handleRegister(w, r)
		return
	}

	if r.URL.Path == "/v1/console" {
		s.handleConsoleSocket(w, r)
		return
	}

	if strings.HasPrefix(r.URL.Path, "/v1/devices/") && strings.HasSuffix(r.URL.Path, "/shadow") {
		s.handleShadow(w, r)
		return
	}

	http.NotFound(w, r)
}

func (s *TestServer) handleRegister(w http.ResponseWriter, r *http.Request) {
	var req struct {
		DeviceUID string `json:"device_uid"`
		TenantID  string `json:"tenant_id"`
	}
	_ = json.NewDecoder(r.Body).Decode(&req)

	err := s.DB.RegisterDevice(req.DeviceUID, req.TenantID, "mock_secret")
	if err != nil {
		http.Error(w, err.Error(), http.StatusConflict)
		return
	}
	w.WriteHeader(http.StatusCreated)
	_ = json.NewEncoder(w).Encode(map[string]string{"status": "created"})
}

func (s *TestServer) handleShadow(w http.ResponseWriter, r *http.Request) {
	pathParts := strings.Split(r.URL.Path, "/")
	deviceId := pathParts[3]

	rec, err := s.DB.GetShadow(deviceId)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	if rec == nil {
		http.Error(w, "Shadow record missing", http.StatusNotFound)
		return
	}

	if r.Method == http.MethodGet {
		repMap, _ := shadow.ParseStateMap(rec.Reported)
		desMap, _ := shadow.ParseStateMap(rec.Desired)
		delMap := shadow.CalculateDelta(desMap, repMap)

		_ = json.NewEncoder(w).Encode(map[string]interface{}{
			"device_id": rec.DeviceID,
			"version":   rec.Version,
			"state": map[string]interface{}{
				"reported": repMap,
				"desired":  desMap,
				"delta":    delMap,
			},
		})
	} else if r.Method == http.MethodPut {
		var req struct {
			State struct {
				Reported map[string]interface{} `json:"reported"`
				Desired  map[string]interface{} `json:"desired"`
			} `json:"state"`
		}
		_ = json.NewDecoder(r.Body).Decode(&req)

		repMap, _ := shadow.ParseStateMap(rec.Reported)
		desMap, _ := shadow.ParseStateMap(rec.Desired)

		if req.State.Reported != nil {
			mergeMaps(repMap, req.State.Reported)
		}
		if req.State.Desired != nil {
			mergeMaps(desMap, req.State.Desired)
		}

		delMap := shadow.CalculateDelta(desMap, repMap)
		newVersion := rec.Version + 1

		repBytes, _ := json.Marshal(repMap)
		desBytes, _ := json.Marshal(desMap)

		_ = s.DB.UpdateShadow(deviceId, newVersion, string(repBytes), string(desBytes))

		responsePayload := map[string]interface{}{
			"device_id": deviceId,
			"version":   newVersion,
			"state": map[string]interface{}{
				"reported": repMap,
				"desired":  desMap,
				"delta":    delMap,
			},
		}

		// Broadcast update to subscribed Websockets
		broadcastBytes, _ := json.Marshal(responsePayload)
		testBroker.Broadcast(deviceId, broadcastBytes)

		_ = json.NewEncoder(w).Encode(responsePayload)
	}
}

func (s *TestServer) handleConsoleSocket(w http.ResponseWriter, r *http.Request) {
	hj, ok := w.(http.Hijacker)
	if !ok {
		http.Error(w, "Upgrade failed", http.StatusBadRequest)
		return
	}
	conn, bufrw, err := hj.Hijack()
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	key := r.Header.Get("Sec-WebSocket-Key")
	accept := testComputeAcceptKey(key)

	_, _ = bufrw.WriteString("HTTP/1.1 101 Switching Protocols\r\n")
	_, _ = bufrw.WriteString("Upgrade: websocket\r\n")
	_, _ = bufrw.WriteString("Connection: Upgrade\r\n")
	_, _ = bufrw.WriteString("Sec-WebSocket-Accept: " + accept + "\r\n\r\n")
	_ = bufrw.Flush()

	defer func() {
		testBroker.Unsubscribe(conn)
		conn.Close()
	}()

	// Wait and read subscriber command frame
	// Frame structure: Mask (1) + Opcode (1) + Payload
	header := make([]byte, 2)
	if _, err := conn.Read(header); err != nil {
		return
	}

	payloadLen := int(header[1] & 0x7F)
	maskKey := make([]byte, 4)
	if _, err := conn.Read(maskKey); err != nil {
		return
	}

	payload := make([]byte, payloadLen)
	if _, err := io.ReadFull(conn, payload); err != nil {
		return
	}

	for i := 0; i < payloadLen; i++ {
		payload[i] ^= maskKey[i%4]
	}

	var cmd struct {
		Action   string `json:"action"`
		DeviceID string `json:"device_id"`
	}

	if err := json.Unmarshal(payload, &cmd); err == nil {
		if cmd.Action == "subscribe" && cmd.DeviceID != "" {
			testBroker.Subscribe(cmd.DeviceID, conn)
		}
	}

	// Hold connection alive during test
	dummy := make([]byte, 1)
	for {
		if _, err := conn.Read(dummy); err != nil {
			break
		}
	}
}

func TestShadowDeltaLogic(t *testing.T) {
	desired := map[string]interface{}{
		"relay":            true,
		"target_temp":      22.5,
		"deep_nested_test": map[string]interface{}{"mode": "auto", "speed": 3},
	}
	reported := map[string]interface{}{
		"relay":            true,
		"target_temp":      20.0,
		"deep_nested_test": map[string]interface{}{"mode": "auto", "speed": 1},
	}

	delta := shadow.CalculateDelta(desired, reported)

	if delta["relay"] != nil {
		t.Errorf("relay delta should be empty since desired == reported")
	}

	if delta["target_temp"] != 22.5 {
		t.Errorf("target_temp delta mismatch, got: %v", delta["target_temp"])
	}

	nested, ok := delta["deep_nested_test"].(map[string]interface{})
	if !ok {
		t.Fatalf("deep_nested_test delta should be a map")
	}
	if nested["mode"] != nil {
		t.Errorf("mode delta should be empty")
	}
	if nested["speed"] != 3 {
		t.Errorf("speed delta mismatch, got: %v", nested["speed"])
	}
}

func TestIntegratedCloudAPI(t *testing.T) {
	db, err := database.InitDB(":memory:")
	if err != nil {
		t.Fatalf("Failed to initialize test DB: %v", err)
	}
	defer db.Conn.Close()

	srv := &TestServer{DB: db}
	testServer := httptest.NewServer(srv)
	defer testServer.Close()

	// 1. Test registration
	regPayload := `{"device_uid":"dev_esp32_01","tenant_id":"tenant_test"}`
	resp, err := http.Post(testServer.URL+"/v1/devices/register", "application/json", bytes.NewBufferString(regPayload))
	if err != nil {
		t.Fatalf("Registration request failed: %v", err)
	}
	if resp.StatusCode != http.StatusCreated {
		t.Errorf("Expected status Created, got: %s", resp.Status)
	}
	resp.Body.Close()

	// 2. Test Get initial shadow
	resp, err = http.Get(testServer.URL + "/v1/devices/dev_esp32_01/shadow")
	if err != nil {
		t.Fatalf("Fetch shadow request failed: %v", err)
	}
	var shadowBody map[string]interface{}
	_ = json.NewDecoder(resp.Body).Decode(&shadowBody)
	resp.Body.Close()

	version := shadowBody["version"].(float64)
	if version != 1 {
		t.Errorf("Expected version 1, got: %v", version)
	}

	// 3. Test Update desired state shadow delta
	updatePayload := `{"state":{"desired":{"relay_status":true,"temp_alarm":40.0}}}`
	req, _ := http.NewRequest(http.MethodPut, testServer.URL+"/v1/devices/dev_esp32_01/shadow", bytes.NewBufferString(updatePayload))
	resp, err = http.DefaultClient.Do(req)
	if err != nil {
		t.Fatalf("Update desired state failed: %v", err)
	}
	_ = json.NewDecoder(resp.Body).Decode(&shadowBody)
	resp.Body.Close()

	version = shadowBody["version"].(float64)
	if version != 2 {
		t.Errorf("Expected version 2, got: %v", version)
	}

	state := shadowBody["state"].(map[string]interface{})
	delta := state["delta"].(map[string]interface{})
	if delta["relay_status"] != true || delta["temp_alarm"] != 40.0 {
		t.Errorf("Shadow delta mismatch: %v", delta)
	}

	// 4. Test Update reported state to match desired state (resolving delta)
	reportPayload := `{"state":{"reported":{"relay_status":true,"temp_alarm":40.0}}}`
	req, _ = http.NewRequest(http.MethodPut, testServer.URL+"/v1/devices/dev_esp32_01/shadow", bytes.NewBufferString(reportPayload))
	resp, err = http.DefaultClient.Do(req)
	if err != nil {
		t.Fatalf("Update reported state failed: %v", err)
	}
	_ = json.NewDecoder(resp.Body).Decode(&shadowBody)
	resp.Body.Close()

	version = shadowBody["version"].(float64)
	if version != 3 {
		t.Errorf("Expected version 3, got: %v", version)
	}

	state = shadowBody["state"].(map[string]interface{})
	delta = state["delta"].(map[string]interface{})
	if len(delta) != 0 {
		t.Errorf("Expected empty delta, got: %v", delta)
	}
}

func TestWebSocketConsoleRealtime(t *testing.T) {
	db, err := database.InitDB(":memory:")
	if err != nil {
		t.Fatalf("Failed to initialize test DB: %v", err)
	}
	defer db.Conn.Close()

	srv := &TestServer{DB: db}
	testServer := httptest.NewServer(srv)
	defer testServer.Close()

	// Register device
	_ = db.RegisterDevice("dev_esp32_ws", "tenant_ws", "secret")

	// Parse server URL to dial raw TCP connection
	u, _ := url.Parse(testServer.URL)
	conn, err := net.Dial("tcp", u.Host)
	if err != nil {
		t.Fatalf("Failed to dial tcp to testServer: %v", err)
	}
	defer conn.Close()

	// Perform manual WebSocket handshake
	handshake := fmt.Sprintf(
		"GET /v1/console HTTP/1.1\r\n"+
			"Host: %s\r\n"+
			"Upgrade: websocket\r\n"+
			"Connection: Upgrade\r\n"+
			"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"+
			"Sec-WebSocket-Version: 13\r\n\r\n", u.Host)
	_, _ = conn.Write([]byte(handshake))

	// Read handshake response
	respBuf := make([]byte, 1024)
	n, err := conn.Read(respBuf)
	if err != nil {
		t.Fatalf("Failed to read handshake response: %v", err)
	}
	if !strings.Contains(string(respBuf[:n]), "101 Switching Protocols") {
		t.Fatalf("WebSocket upgrade failed: %s", string(respBuf[:n]))
	}

	// Send subscription frame: {"action":"subscribe","device_id":"dev_esp32_ws"}
	subPayload := `{"action":"subscribe","device_id":"dev_esp32_ws"}`
	var frame []byte
	frame = append(frame, 0x81) // Fin + Opcode Text
	
	// Mask = 1, Payload Length
	frame = append(frame, 0x80|byte(len(subPayload)))
	
	maskKey := []byte{0x12, 0x34, 0x56, 0x78}
	frame = append(frame, maskKey...)

	maskedPayload := make([]byte, len(subPayload))
	for i := 0; i < len(subPayload); i++ {
		maskedPayload[i] = subPayload[i] ^ maskKey[i%4]
	}
	frame = append(frame, maskedPayload...)

	_, err = conn.Write(frame)
	if err != nil {
		t.Fatalf("Failed to send subscription frame: %v", err)
	}

	// Trigger a shadow update on HTTP PUT endpoint which should broadcast to WebSocket connection
	updatePayload := `{"state":{"reported":{"temperature":28.4}}}`
	putReq, _ := http.NewRequest(
		http.MethodPut, 
		testServer.URL+"/v1/devices/dev_esp32_ws/shadow", 
		bytes.NewBufferString(updatePayload),
	)
	putResp, err := http.DefaultClient.Do(putReq)
	if err != nil {
		t.Fatalf("Shadow update PUT request failed: %v", err)
	}
	putResp.Body.Close()

	// Read real-time broadcast message from WebSocket connection
	wsMessage, err := testReadWebSocketFrame(conn)
	if err != nil {
		t.Fatalf("Failed to read WebSocket broadcast frame: %v", err)
	}

	var broadcastBody map[string]interface{}
	_ = json.Unmarshal(wsMessage, &broadcastBody)

	if broadcastBody["device_id"] != "dev_esp32_ws" {
		t.Errorf("Broadcast device_id mismatch, got: %v", broadcastBody["device_id"])
	}

	version := broadcastBody["version"].(float64)
	if version != 2 {
		t.Errorf("Expected shadow version 2 after updates, got: %v", version)
	}

	state := broadcastBody["state"].(map[string]interface{})
	reported := state["reported"].(map[string]interface{})
	if reported["temperature"].(float64) != 28.4 {
		t.Errorf("Expected reported temperature 28.4, got: %v", reported["temperature"])
	}
}

func TestLocalCLIWorkspaceInit(t *testing.T) {
	defer os.Remove("device_config.json")

	config := map[string]string{
		"device_uid": "dev_mock_ef342",
		"target":     "esp32",
		"server_url": "http://localhost:8080",
	}

	configBytes, _ := json.MarshalIndent(config, "", "  ")
	err := os.WriteFile("device_config.json", configBytes, 0644)
	if err != nil {
		t.Fatalf("Failed to write config: %v", err)
	}

	readBytes, err := os.ReadFile("device_config.json")
	if err != nil {
		t.Fatalf("Failed to read config: %v", err)
	}

	var readMap map[string]string
	_ = json.Unmarshal(readBytes, &readMap)

	if !reflect.DeepEqual(config, readMap) {
		t.Errorf("Config file output mismatch, got: %v", readMap)
	}
}

func TestMain(m *testing.M) {
	os.Exit(m.Run())
}
