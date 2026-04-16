import serial
import time
import math

SERIAL_PORT = 'COM13' 
BAUD_RATE = 115200
CAR_ID = 20 

CENTER_LAT = 55.7126
CENTER_LON = 13.2091
RADIUS_METERS = 100
LAT_DEGREE_LEN = 111320.0
LON_DEGREE_LEN = 40075000.0 * math.cos(math.radians(CENTER_LAT)) / 360.0

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05) 
    time.sleep(0.1) 
    
    angle = 0
    print(f"--- Python Controller Active for ID {CAR_ID} ---")
    counter =0
    start_time = time.time()

    while True:
        # 1. Calculate Coordinates
        lat_offset = (RADIUS_METERS * math.sin(math.radians(angle))) / LAT_DEGREE_LEN
        lon_offset = (RADIUS_METERS * math.cos(math.radians(angle))) / LON_DEGREE_LEN
        
        # 2. Generate HHMMSS.ss timestamp
        # Example: 14:30:05.50 -> 14300550
        t = time.localtime()
        hundredths = int((time.time() % 1) * 100)
        timestamp = (t.tm_hour * 1000000) + (t.tm_min * 10000) + (t.tm_sec * 100) + hundredths
        
        # 3. Set TTL
        ttl = 2
        
        # 4. Create Payload: ID,LAT,LON,TIMESTAMP,TTL
        payload = f"{CAR_ID},{CENTER_LAT + lat_offset:.6f},{CENTER_LON + lon_offset:.6f},{timestamp},{ttl}\n"
        ser.write(payload.encode('utf-8'))
        counter += 1
        if counter % 10 == 0:
            print(f"Sent {counter} messages")
            print(f"Seconds {time.time() - start_time:.2f}: {counter} messages")



        # Check for feedback from ESP32
        while ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            print(f"ESP32: {line}")
        
        angle = (angle + 5) % 360
        time.sleep(0.5)

except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals(): ser.close()