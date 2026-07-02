package main

import (
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"flag"
	"log"
	"net/http"
	"strings"

	"deviceos/cloud/pkg/database"
	"deviceos/cloud/pkg/shadow"
)

type RegisterReq struct {
	DeviceUID string `json:"device_uid"`
	TenantID  string `json:"tenant_id"`
}

type ShadowUpdateReq struct {
	State struct {
		Reported map[string]interface{} `json:"reported"`
		Desired  map[string]interface{} `json:"desired"`
	} `json:"state"`
}

type Server struct {
	DB *database.DB
}

func generateSecret() string {
	b := make([]byte, 16)
	_, _ = rand.Read(b)
	return hex.EncodeToString(b)
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

func (s *Server) handleRegister(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req RegisterReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Bad request", http.StatusBadRequest)
		return
	}

	if req.DeviceUID == "" || req.TenantID == "" {
		http.Error(w, "device_uid and tenant_id are required", http.StatusBadRequest)
		return
	}

	token := generateSecret()
	if err := s.DB.RegisterDevice(req.DeviceUID, req.TenantID, token); err != nil {
		http.Error(w, err.Error(), http.StatusConflict)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	_ = json.NewEncoder(w).Encode(map[string]string{
		"device_uid":   req.DeviceUID,
		"tenant_id":    req.TenantID,
		"secret_token": token,
	})
}

func (s *Server) handleListDevices(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	devices, err := s.DB.ListDevices()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// For MVP, if there are no devices, return an empty array instead of null
	if devices == nil {
		devices = []string{}
	}

	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	_ = json.NewEncoder(w).Encode(map[string]interface{}{
		"devices": devices,
	})
}

func (s *Server) handleShadow(w http.ResponseWriter, r *http.Request) {
	pathParts := strings.Split(r.URL.Path, "/")
	if len(pathParts) < 5 || pathParts[2] != "devices" || pathParts[4] != "shadow" {
		http.NotFound(w, r)
		return
	}
	deviceId := pathParts[3]

	dev, err := s.DB.GetDevice(deviceId)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	if dev == nil {
		http.Error(w, "Device not registered", http.StatusNotFound)
		return
	}

	switch r.Method {
	case http.MethodGet:
		s.getShadowHandler(w, deviceId)
	case http.MethodPut:
		s.putShadowHandler(w, r, deviceId)
	default:
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

func (s *Server) getShadowHandler(w http.ResponseWriter, deviceId string) {
	rec, err := s.DB.GetShadow(deviceId)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	if rec == nil {
		http.Error(w, "Shadow record missing", http.StatusNotFound)
		return
	}

	repMap, _ := shadow.ParseStateMap(rec.Reported)
	desMap, _ := shadow.ParseStateMap(rec.Desired)
	delMap := shadow.CalculateDelta(desMap, repMap)

	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string]interface{}{
		"device_id": rec.DeviceID,
		"version":   rec.Version,
		"state": map[string]interface{}{
			"reported": repMap,
			"desired":  desMap,
			"delta":    delMap,
		},
		"updated_at": rec.UpdatedAt,
	})
}

func (s *Server) putShadowHandler(w http.ResponseWriter, r *http.Request, deviceId string) {
	var req ShadowUpdateReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Bad request", http.StatusBadRequest)
		return
	}

	rec, err := s.DB.GetShadow(deviceId)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	if rec == nil {
		http.Error(w, "Shadow record missing", http.StatusNotFound)
		return
	}

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

	err = s.DB.UpdateShadow(deviceId, newVersion, string(repBytes), string(desBytes))
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	responsePayload := map[string]interface{}{
		"device_id": deviceId,
		"version":   newVersion,
		"state": map[string]interface{}{
			"reported": repMap,
			"desired":  desMap,
			"delta":    delMap,
		},
	}

	// Broadcast updated shadow directly to WebSockets
	broadcastBytes, _ := json.Marshal(responsePayload)
	GlobalBroker.Broadcast(deviceId, broadcastBytes)

	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(responsePayload)
}

func main() {
	port := flag.String("port", "0.0.0.0:8082", "HTTP server port binding")
	dbPath := flag.String("db", "deviceos.db", "SQLite database path")
	flag.Parse()

	log.Printf("Connecting database path: %s", *dbPath)
	db, err := database.InitDB(*dbPath)
	if err != nil {
		log.Fatalf("Database initialization failed: %v", err)
	}
	defer db.Conn.Close()

	srv := &Server{DB: db}

	// Routes
	http.HandleFunc("/v1/devices/register", srv.handleRegister)
	http.HandleFunc("/v1/devices", srv.handleListDevices)
	http.HandleFunc("/v1/devices/", srv.handleShadow)
	http.HandleFunc("/v1/console", srv.handleConsoleSocket) // WebSocket endpoint

	log.Printf("Starting DeviceOS Registry Gateway server on %s", *port)
	if err := http.ListenAndServe(*port, nil); err != nil {
		log.Fatalf("Server listen failed: %v", err)
	}
}
