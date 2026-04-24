import tkinter as tk
from tkinter import scrolledtext
from tkinter import messagebox
from tkinter import ttk
import time
import threading
import math
import serial

# -----------------------
# CAR ID (selected car)
# -----------------------
car_id = 1

# -----------------------
# SERIAL SETUP (USB ESPs)
# -----------------------
serials = {}

ESP_PORTS = {
    1: "COM13",
    2: "COM13",  # ESP #1
}

def read_from_esp(cid):
    ser = serials.get(cid)
    if not ser:
        return

    print(f"--- Listening ESP {cid} ---")

    while True:
        try:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    print(f"ESP{cid}: {line}")

        except Exception as e:
            print(f"ESP {cid} read error: {e}")
            break
def connect_esp_ports():
    for cid, port in ESP_PORTS.items():
        try:
            ser = serial.Serial(port, 115200, timeout=0.1)
            serials[cid] = ser

            time.sleep(2)  # ESP reset
            ser.reset_input_buffer()

            print(f"Connected ESP {cid} on {port}")

            threading.Thread(
                target=read_from_esp,
                args=(cid,),
                daemon=True
            ).start()

        except Exception as e:
            print(f"Failed to connect {port}: {e}")

# -----------------------
# FLAGS
# -----------------------
stop_flag = False

# -----------------------
# SEND FUNCTION
# -----------------------
def send_points():
    global stop_flag
    print("FUNCTION RUNNING")
    raw_text = coordBox.get("1.0", tk.END).strip()

    if not raw_text:
        print("No coordinates")
        return

    try:
        points = eval(raw_text)  # replace with json later if needed
    except:
        print("Invalid format")
        return

    progress["maximum"] = len(points) - 1
    progress["value"] = 0
    stop_flag = False

    for i in range(len(points) - 1):
        if stop_flag:
            print("Stopped")
            break

        progress["value"] = i + 1

        lon1, lat1 = points[i]
        lon2, lat2 = points[i + 1]

        # distance estimate
        meters_per_deg = 111320
        meters_lon = (lon2 - lon1) * meters_per_deg * math.cos(math.radians(lat1))
        meters_lat = (lat2 - lat1) * meters_per_deg
        distance = math.sqrt(meters_lon**2 + meters_lat**2)

        speed = speed_var.get()
        delay = distance / speed if speed > 0 else 1

        # -----------------------
        # FORMAT FOR ESP:
        # lat,lon,timestamp,ttl
        # -----------------------
        timestamp = int(time.time() * 1000)
        ttl = 2

        msg = f"{1},{lat1},{lon1},{timestamp},{ttl}\n"
        # send to selected ESP via USB
        if car_id in serials:

            try:
                serials[car_id].write(msg.encode())
                print(f"Sent to ESP {car_id}: {msg.strip()}")
                
            except Exception as e:
                print("Send error:", e)

        time.sleep(delay)

    root.after(0, lambda: progress.config(value=0))

# -----------------------
# DISTANCE FUNCTION
# -----------------------
def calculate_total_distance():
    raw_text = coordBox.get("1.0", tk.END).strip()

    try:
        points = eval(raw_text)
    except:
        messagebox.showerror("Error", "Invalid format")
        return

    if len(points) < 2:
        messagebox.showinfo("Info", "Need at least 2 points")
        return

    total = 0
    meters_per_deg = 111320

    for i in range(len(points) - 1):
        lon1, lat1 = points[i]
        lon2, lat2 = points[i + 1]

        meters_lon = (lon2 - lon1) * meters_per_deg * math.cos(math.radians(lat1))
        meters_lat = (lat2 - lat1) * meters_per_deg
        total += math.sqrt(meters_lon**2 + meters_lat**2)

    messagebox.showinfo("Distance", f"Total: {total:.1f} meters")

# -----------------------
# CONTROL FUNCTIONS
# -----------------------
def start_sending():
    t = threading.Thread(target=send_points)
    t.start()

def stop_sending():
    global stop_flag
    stop_flag = True

def open_selector():
    def select():
        global car_id
        car_id = int(combo.get())
        top.destroy()

    top = tk.Toplevel(root)
    top.title("Select ESP / Car ID")

    combo = ttk.Combobox(top, values=list(range(1, 3)))
    combo.pack(padx=20, pady=20)

    tk.Button(top, text="Select", command=select).pack(pady=10)

# -----------------------
# GUI
# -----------------------
root = tk.Tk()
root.title("USB GNSS Sender")
root.configure(bg="black")

main_frame = tk.Frame(root, bg="black")
main_frame.pack(fill="both", expand=True)

left = tk.Frame(main_frame, bg="black")
left.pack(side="left", fill="y")

right = tk.Frame(main_frame, bg="black")
right.pack(side="left", fill="both", expand=True)

# SPEED
speed_var = tk.DoubleVar(value=10)

tk.Label(left, text="Speed (m/s)", fg="white", bg="black").pack()
tk.Scale(left, from_=0.1, to=50, orient="horizontal", variable=speed_var).pack()

# BUTTONS
tk.Button(left, text="Start", command=start_sending).pack(fill="x")
tk.Button(left, text="Stop", command=stop_sending).pack(fill="x")
tk.Button(left, text="Select Car", command=open_selector).pack(fill="x")
tk.Button(left, text="Distance", command=calculate_total_distance).pack(fill="x")

# PROGRESS
progress = ttk.Progressbar(left, orient="horizontal", mode="determinate")
progress.pack(fill="x")

# TEXT BOX
coordBox = scrolledtext.ScrolledText(right, width=60, height=25, bg="gray")
coordBox.pack(fill="both", expand=True)

# -----------------------
# START SERIAL CONNECTION
# -----------------------
connect_esp_ports()

root.mainloop()