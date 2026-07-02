import requests
import json
import time
import subprocess

BASE_URL = "http://localhost:9001"

def test_simulator():
    print("--- Testing Simulator Provisioning ---")
    
    print("1. Triggering Provisioning...")
    payload = {
        "ssid": "mywifi",
        "password": "secretpassword",
        "target_endpoint": "10.101.104.32:8082",
        "tenant_id": "tenant1"
    }
    
    try:
        r = requests.post(f"{BASE_URL}/provision", json=payload)
        print(f"Provision Status: {r.status_code}, Body: {r.text}")
        assert r.status_code == 200, "Provision failed"
        print("SIMULATOR TEST PASS")
    except Exception as e:
        print(f"Simulator Test Error: {e}")

if __name__ == "__main__":
    test_simulator()
