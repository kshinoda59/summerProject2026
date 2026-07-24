import csv
import time
import serial

PORT = "COM5"
BAUD_RATE = 460800
RECORD_SECONDS = 10

print(f"Opening {PORT}...")

with serial.Serial(PORT, BAUD_RATE, timeout=1) as ser:

    # Give the ESP32 a moment to reset
    time.sleep(2)

    print(f"Recording for {RECORD_SECONDS} seconds...")

    with open("sensor_data.csv", "w", newline="") as file:

        writer = csv.writer(file)

        # CSV header
        writer.writerow([
            "timestamp_us",
            "ax",
            "ay",
            "az",
            "gx",
            "gy",
            "gz",
            "dbfs"
        ])

        start = time.time()
        rows = 0

        while time.time() - start < RECORD_SECONDS:

            line = ser.readline().decode(
                "utf-8",
                errors="ignore"
            ).strip()

            if not line:
                continue

            values = line.split(",")

            if len(values) == 8:
                writer.writerow(values)
                rows += 1

print(f"Done! Saved {rows} rows to sensor_data.csv")