import serial
import time
import math

# --- CONFIGURATION ---
SERIAL_PORT = 'COM5' 
BAUD_RATE = 115200
CAR_ID = 20

# LTH Center
CENTER_LAT = 55.7126
CENTER_LON = 13.2091
RADIUS_METERS = 100
LAT_DEGREE_LEN = 111320.0
LON_DEGREE_LEN = 40075000.0 * math.cos(math.radians(CENTER_LAT)) / 360.0

try:
    # Adding a longer timeout helps Python not "hang" if the ESP is slow
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1) 
    time.sleep(2) 
    
    angle = 0
    print(f"--- Connected to {SERIAL_PORT} ---")
    
    while True:
        # Calculate Circle
        lat_offset = (RADIUS_METERS * math.sin(math.radians(angle))) / LAT_DEGREE_LEN
        lon_offset = (RADIUS_METERS * math.cos(math.radians(angle))) / LON_DEGREE_LEN
        
        payload = f"{CAR_ID},{CENTER_LAT + lat_offset:.6f},{CENTER_LON + lon_offset:.6f}\n"
        ser.write(payload.encode('utf-8'))

        # READ THE ESP32 OUTPUT (This replaces the Arduino Serial Monitor)
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            print(f"ESP32 Status: {line}")
        
        angle = (angle + 5) % 360
        time.sleep(0.5)

except serial.SerialException:
    print("Error: Port busy! Close Arduino Serial Monitor.")
except KeyboardInterrupt:
    print("Stopped.")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()