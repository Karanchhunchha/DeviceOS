import requests
import json
import time
import random

BASE_URL = "http://localhost:8082"
DEVICE_ID = "esp32_device_001"

def register_device():
    print(f"Registering Device: {DEVICE_ID}...")
    payload = {
        "device_uid": DEVICE_ID,
        "tenant_id": "tenant_default",
        "secret_token": "esp32_secret"
    }
    r = requests.post(f"{BASE_URL}/v1/devices/register", json=payload)
    if r.status_code == 201:
        print("Device registered successfully!")
    elif r.status_code == 409:
        print("Device already registered.")
    else:
        print(f"Failed to register. Status: {r.status_code}")

def send_telemetry_loop():
    print(f"\nStarting live telemetry stream for {DEVICE_ID}...")
    print("Press Ctrl+C to stop.\n")
    
    # Starting values
    temp = 24.5
    humidity = 45.0
    
    try:
        while True:
            # Add slight random variations
            temp += random.uniform(-0.5, 0.5)
            humidity += random.uniform(-1.0, 1.0)
            
            # Keep within logical bounds
            temp = max(15.0, min(35.0, temp))
            humidity = max(20.0, min(80.0, humidity))
            
            payload = {
                "state": {
                    "reported": {
                        "temperature": round(temp, 1),
                        "humidity": round(humidity, 1)
                    }
                }
            }
            
            r = requests.put(f"{BASE_URL}/v1/devices/{DEVICE_ID}/shadow", json=payload)
            if r.status_code == 200:
                print(f"Sent Data -> Temp: {round(temp, 1)} C | Humidity: {round(humidity, 1)}%")
            else:
                print(f"Failed to send data: {r.text}")
                
            # Send data every 2 seconds
            time.sleep(2)
            
    except KeyboardInterrupt:
        print("\nStopped telemetry stream.")

if __name__ == "__main__":
    # Ensure server is running before executing this
    register_device()
    send_telemetry_loop()
