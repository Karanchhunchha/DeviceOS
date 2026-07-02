import asyncio
from bleak import BleakScanner

SERVICE_UUID = "4b500000-0000-0000-0000-000000004f23"

async def main():
    print("Starting BLE scan for 10 seconds...")
    print(f"Looking for DeviceOS UUID: {SERVICE_UUID}")
    
    # discover returns a dict: { address: (BLEDevice, AdvertisementData) }
    devices = await BleakScanner.discover(timeout=10.0, return_adv=True)
    found = False
    
    for address, (device, adv_data) in devices.items():
        # Check if the specific UUID is in the advertised service UUIDs
        uuids = adv_data.service_uuids
        if SERVICE_UUID in [str(u).lower() for u in uuids]:
            print("\n" + "="*50)
            print("SUCCESS! FOUND DEVICEOS APP BROADCASTING!")
            print(f"Name: {device.name}")
            print(f"MAC Address: {address}")
            print(f"Signal Strength (RSSI): {adv_data.rssi} dBm")
            print("="*50 + "\n")
            found = True
            
    if not found:
        print("\nCould not find any device advertising the DeviceOS UUID.")
        print("Please make sure:")
        print("1. Your phone has 'Host Peer Mode' ON and 'Start BLE Advertisement' clicked.")
        print("2. Your laptop's Bluetooth is turned ON.")
        print("3. Bring the phone close to the laptop.")

if __name__ == "__main__":
    asyncio.run(main())
