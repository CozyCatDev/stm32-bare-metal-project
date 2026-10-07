import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import serial


# ============================================================
# Serial configuration
# ============================================================

PORT = 'COM5'
BAUDRATE = 115200

ser = serial.Serial(
    PORT,
    BAUDRATE,
    timeout=1
)


# ============================================================
# Measurement configuration
# ============================================================

NUM_MEASUREMENTS = 10
SAMPLE_SIZE = 9
BATCH_SIZE = NUM_MEASUREMENTS * SAMPLE_SIZE


# ============================================================
# Data generator
# ============================================================

def data_generator():

    while True:

        # ----------------------------------------------------
        # Request 10 measurements
        # FD = start
        # 04 = command
        # 0A = number of measurements
        # FE = end
        # ----------------------------------------------------

        ser.write(bytes([
            0xFD,
            4,
            NUM_MEASUREMENTS,
            0xFE
        ]))

        # ----------------------------------------------------
        # Receive exactly 90 bytes
        # ----------------------------------------------------

        raw_data = ser.readline().rstrip(b'\n')

        if len(raw_data) != BATCH_SIZE:
            print(
                f"Incomplete batch: "
                f"received {len(raw_data)}/{BATCH_SIZE} bytes"
            )
            continue

        # ----------------------------------------------------
        # Process each 9-byte sample
        # ----------------------------------------------------

        for i in range(0, BATCH_SIZE, SAMPLE_SIZE):

            # Accelerometer
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
                f"X={x:6d} "
                f"Y={y:6d} "
                f"Z={z:6d} "
                f"P={pressure:3d} "
                f"T={temperature:3d} "
                f"H={humidity:3d}"
            )

            # Return one sample at a time
            yield (
                x,
                y,
                z,
                pressure,
                temperature,
                humidity
            )



# ============================================================
# Figure
# ============================================================

fig = plt.figure(figsize=(12, 6))

ax_accel = fig.add_subplot(
    1,
    2,
    1,
    projection='3d'
)

ax_env = fig.add_subplot(
    1,
    2,
    2
)

fig.suptitle("STM32 Sensor Monitor")


# ============================================================
# Accelerometer subplot
# ============================================================

ax_accel.set_title("Accelerometer")

ax_accel.set_xlabel("X")
ax_accel.set_ylabel("Y")
ax_accel.set_zlabel("Z")

ax_accel.set_xlim(
    -500,
    500
)

ax_accel.set_ylim(
    -500,
    500
)

ax_accel.set_zlim(
    500,
    1500
)

# Initial position of dot
dot, = ax_accel.plot(
    [0],
    [0],
    [0],
    color='red',
    marker='o',
    markersize=5,
    linestyle=''
)


# ============================================================
# Environmental subplot
# ============================================================

environment_labels = [
    "Pressure",
    "Temperature",
    "Humidity"
]

environment_values = [
    0,
    0,
    0
]

bars = ax_env.bar(
    environment_labels,
    environment_values,
    color=["purple", "orange", "blue"]
)

ax_env.set_title("Environmental Data")
ax_env.set_ylabel("Value")
ax_env.grid(
    axis='y',
    alpha=0.3
)


# ============================================================
# Animation update
# ============================================================

def update(data):

    (
        x,
        y,
        z,
        pressure,
        temperature,
        humidity
    ) = data

    # --------------------------------------------------------
    # Move 3D accelerometer dot
    # --------------------------------------------------------

    dot.set_data(
        [x],
        [y]
    )

    dot.set_3d_properties(
        [z]
    )

    # --------------------------------------------------------
    # Update environmental bar chart
    # --------------------------------------------------------

    environment_values = [
        pressure,
        temperature,
        humidity
    ]

    for bar, value in zip(
        bars,
        environment_values
    ):
        bar.set_height(value)

    # Update environmental axis
    ax_env.relim()
    ax_env.autoscale_view()

    # --------------------------------------------------------
    # Display values above bars
    # --------------------------------------------------------

    units = ["kPa", "°C", "%RH"]

    # Remove existing value labels
    for text in ax_env.texts:
        text.remove()

    for bar, value, unit in zip(
        bars,
        environment_values,
        units
    ):

        ax_env.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height(),
            f"{value}{unit}",
            ha='center',
            va='bottom'
        )

    return (
        dot,
        *bars
    )


# ============================================================
# Start animation
# ============================================================

ani = FuncAnimation(
    fig,
    update,
    frames=data_generator(),
    interval=0,
    blit=False,
    cache_frame_data=False
)


# ============================================================
# Display
# ============================================================

plt.tight_layout()
plt.show()


# ============================================================
# Close serial port when finished
# ============================================================

ser.close()