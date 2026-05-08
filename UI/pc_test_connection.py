import socket
import time

# Remember to connect your PC to the same Wi-Fi network as the ESP32 and update the IP address below!
ESP32_IP = "192.168.0.132" # Replace with the IP shown in your Serial Monitor
PORT = 5000

print(f"Attempting to connect to {ESP32_IP}...")

try:
    # Create socket and connect
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.settimeout(5) # 5 second timeout
    client.connect((ESP32_IP, PORT))
    
    print("SUCCESS: Connected to ESP32!")
    print("The onboard LED should be blinking fast now.")
    
    # Listen for heartbeats for 10 seconds
    end_time = time.time() + 10
    while time.time() < end_time:
        data = client.recv(1024).decode('utf-8')
        if data:
            print(f"Received from ESP32: {data.strip()}")
            
    # Send a command back
    print("Sending 'OFF' command...")
    client.send("OFF\n".encode())
    
except Exception as e:
    print(f"FAILED: {e}")
finally:
    client.close()
    print("Connection closed.")