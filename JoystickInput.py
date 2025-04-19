import struct
import serial
import time
import pygame

# Setup serial port
ser = serial.Serial('COM7', 115200, timeout=1)  # Change COM7 if needed
time.sleep(2)  # Give Arduino time to reset

# Setup joystick
pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("No joystick found.")
    exit()

joystick = pygame.joystick.Joystick(0)
joystick.init()

print("Joystick:", joystick.get_name())

try:
    while True:
        pygame.event.pump()
        roll = joystick.get_axis(0)
        pitch = joystick.get_axis(1)

        payload = struct.pack('<iff', 0, roll, pitch)  # ID=0
        ser.write(payload)

        print(f"Sent: ID=0, roll={roll:.2f}, pitch={pitch:.2f}")
        time.sleep(0.05)

except KeyboardInterrupt:
    print("Exiting...")
    pygame.quit()
    ser.close()



