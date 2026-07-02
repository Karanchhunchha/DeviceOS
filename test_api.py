import requests
import json
import time
import websocket
import _thread as thread
import sys

BASE_URL = "http://localhost:8082"
WS_URL = "ws://localhost:8082/v1/console"

def test_api():
    print("--- Testing API Endpoints ---")
    
    # 1. Register Device
    print("1. Registering Device...")
    payload = {
        "device_uid": "test-device-api",
        "tenant_id": "tenant1",
        "secret_token": "token123"
    }
    r = requests.post(f"{BASE_URL}/v1/devices/register", json=payload)
    print(f"Register Status: {r.status_code}, Body: {r.text}")
    assert r.status_code == 201, "Register failed"
    
    # 2. Get Shadow
    print("2. Getting Shadow...")
    r = requests.get(f"{BASE_URL}/v1/devices/test-device-api/shadow")
    print(f"Get Shadow Status: {r.status_code}, Body: {r.text}")
    assert r.status_code == 200, "Get Shadow failed"
    
    # 3. Update Shadow
    print("3. Updating Shadow...")
    update_payload = {
        "state": {
            "desired": {"led": "on"}
        }
    }
    r = requests.put(f"{BASE_URL}/v1/devices/test-device-api/shadow", json=update_payload)
    print(f"Update Shadow Status: {r.status_code}, Body: {r.text}")
    assert r.status_code == 200, "Update Shadow failed"
    print("API TEST PASS\n")

def on_message(ws, message):
    print(f"WS Received: {message}")
    # When we receive a message, it means subscribe was successful and we got an update
    print("WEBSOCKET TEST PASS")
    ws.close()

def on_error(ws, error):
    print(f"WS Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("WS Closed")

def on_open(ws):
    def run(*args):
        print("WS Opened. Sending subscribe...")
        subscribe_msg = json.dumps({"action": "subscribe", "device_id": "test-device-api"})
        ws.send(subscribe_msg)
        time.sleep(1)
        
        print("Triggering shadow update to receive push...")
        update_payload = {
            "state": {
                "desired": {"led": "off"}
            }
        }
        requests.put(f"{BASE_URL}/v1/devices/test-device-api/shadow", json=update_payload)
        
    thread.start_new_thread(run, ())

def test_websocket():
    print("--- Testing WebSocket ---")
    ws = websocket.WebSocketApp(WS_URL,
                              on_open=on_open,
                              on_message=on_message,
                              on_error=on_error,
                              on_close=on_close)
    ws.run_forever()

if __name__ == "__main__":
    test_api()
    test_websocket()
