import serial
import time

# Port configurations
PORT = 'COM5'
BAUDRATE = 115200

# Data frame format
num_measurements = 0
bytes_per_measurement = 9;
raw_data = bytes()

# Arrays to store received data
accelerometer_data = []
environmental_data = []

# Initiate UART communication
ser = serial.Serial(PORT, BAUDRATE, timeout=2)
print(f'Connected to {PORT} successfully.')
time.sleep(1)

while(True):
    # User input
    command = int(input("Command: "))

    # Set idle, ADXL345 measurement, BME280 measurement modes
    if command != 4 and command != 5:
        ser.write(bytes([0xFD, command, 0xFE]))

    # Receive ADXL345 and BME280 measurement data
    elif command == 4:
        num_measurements = int(input("Number of measurements: "))
        ser.write(bytes([0xFD, command, num_measurements, 0xFE]))
        raw_data = ser.readline().rstrip(b'\n')
        for i in range(0, num_measurements * bytes_per_measurement, bytes_per_measurement):
            # Accelerometer data
            x = int.from_bytes(
                raw_data[i:i+2],
                byteorder='little',
                signed=True
            )

            y = int.from_bytes(
                raw_data[i+2:i+4],
                byteorder='little',
                signed=True
            )

            z = int.from_bytes(
                raw_data[i+4:i+6],
                byteorder='little',
                signed=True
            )

            # Environmental data
            pressure = raw_data[i+6]
            temperature = raw_data[i+7]
            humidity = raw_data[i+8]

            print(
                f"Measurement #{int(i / bytes_per_measurement + 1)}: "
                f"X={x}mg, "
                f"Y={y}mg, "
                f"Z={z}mg, "
                f"P={pressure}kPa, "
                f"T={temperature}°C, "
                f"H={humidity}%RH"
            )

    # Type a string to LCD
    elif command == 5:
        first_line = input("Enter first line: ")
        second_line = input("Enter second line: ")
        message = '\n'.join([first_line, second_line]).encode('utf-8')
        ser.write(bytes([0xFD, command]) + message + bytes([0xFE]))

    # Sleep 50ms
    time.sleep(0.050)