package main

import (
	"crypto/sha1"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"log"
	"net"
	"net/http"
	"sync"
)

// Broker handles thread-safe WebSocket connections and subscriptions
type BroadcastBroker struct {
	mu        sync.RWMutex
	observers map[string][]net.Conn
}

var GlobalBroker = &BroadcastBroker{
	observers: make(map[string][]net.Conn),
}

func computeAcceptKey(key string) string {
	h := sha1.New()
	h.Write([]byte(key))
	h.Write([]byte("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))
	return base64.StdEncoding.EncodeToString(h.Sum(nil))
}

func UpgradeConnection(w http.ResponseWriter, r *http.Request) (net.Conn, error) {
	hj, ok := w.(http.Hijacker)
	if !ok {
		return nil, fmt.Errorf("webserver does not support connection hijacking")
	}
	conn, bufrw, err := hj.Hijack()
	if err != nil {
		return nil, err
	}

	key := r.Header.Get("Sec-WebSocket-Key")
	accept := computeAcceptKey(key)

	// Write upgrade handshake response directly to TCP stream
	_, _ = bufrw.WriteString("HTTP/1.1 101 Switching Protocols\r\n")
	_, _ = bufrw.WriteString("Upgrade: websocket\r\n")
	_, _ = bufrw.WriteString("Connection: Upgrade\r\n")
	_, _ = bufrw.WriteString("Sec-WebSocket-Accept: " + accept + "\r\n\r\n")
	_ = bufrw.Flush()

	return conn, nil
}

// WriteTextMessage sends a formatted text frame to the socket connection
func WriteTextMessage(conn net.Conn, payload []byte) error {
	length := len(payload)
	var header []byte
	
	// Text Frame opcode 0x81 (Fin=1, Opcode=1)
	header = append(header, 0x81)

	if length <= 125 {
		header = append(header, byte(length))
	} else if length <= 65535 {
		header = append(header, 126)
		header = append(header, byte(length>>8), byte(length))
	} else {
		header = append(header, 127)
		for i := 7; i >= 0; i-- {
			header = append(header, byte(length>>(uint(i)*8)))
		}
	}

	_, err := conn.Write(header)
	if err != nil {
		return err
	}
	_, err = conn.Write(payload)
	return err
}

func ReadWebSocketFrame(conn net.Conn) ([]byte, error) {
	header := make([]byte, 2)
	if _, err := conn.Read(header); err != nil {
		return nil, err
	}

	// Read length
	payloadLen := int(header[1] & 0x7F)
	if payloadLen == 126 {
		lenBuf := make([]byte, 2)
		if _, err := conn.Read(lenBuf); err != nil {
			return nil, err
		}
		payloadLen = int(lenBuf[0])<<8 | int(lenBuf[1])
	}

	// Masking key (always sent by clients)
	maskKey := make([]byte, 4)
	if _, err := conn.Read(maskKey); err != nil {
		return nil, err
	}

	// Payload data
	payload := make([]byte, payloadLen)
	if _, err := conn.Read(payload); err != nil {
		return nil, err
	}

	// Demask payload
	for i := 0; i < payloadLen; i++ {
		payload[i] ^= maskKey[i%4]
	}

	return payload, nil
}

func (b *BroadcastBroker) Subscribe(deviceId string, conn net.Conn) {
	b.mu.Lock()
	defer b.mu.Unlock()
	b.observers[deviceId] = append(b.observers[deviceId], conn)
	log.Printf("[OBSERVER] Subscribed connection to device: %s", deviceId)
}

func (b *BroadcastBroker) Unsubscribe(conn net.Conn) {
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

func (b *BroadcastBroker) Broadcast(deviceId string, data []byte) {
	b.mu.RLock()
	defer b.mu.RUnlock()
	list, exists := b.observers[deviceId]
	if !exists {
		return
	}

	for _, conn := range list {
		go func(c net.Conn) {
			_ = WriteTextMessage(c, data)
		}(conn)
	}
}

// Handler upgrading connection and reading commands
func (srv *Server) handleConsoleSocket(w http.ResponseWriter, r *http.Request) {
	conn, err := UpgradeConnection(w, r)
	if err != nil {
		log.Printf("[WS_UPGRADE] Handshake failed: %v", err)
		http.Error(w, "Upgrade failed", http.StatusBadRequest)
		return
	}
	defer func() {
		GlobalBroker.Unsubscribe(conn)
		conn.Close()
	}()

	for {
		payload, err := ReadWebSocketFrame(conn)
		if err != nil {
			break // Connection closed
		}

		var cmd struct {
			Action   string `json:"action"`
			DeviceID string `json:"device_id"`
		}

		if err := json.Unmarshal(payload, &cmd); err == nil {
			if cmd.Action == "subscribe" && cmd.DeviceID != "" {
				GlobalBroker.Subscribe(cmd.DeviceID, conn)
			}
		}
	}
}
