import struct
import serial
import time
import pygame

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

pygame.init()
pygame.joystick.init()

pb2 = False
pb3 = False
pb14 = False
pb12= False
pb13 = False

last_roll = 0
last_pitch = 0
last_throttle = 0

if pygame.joystick.get_count() == 0:
    print("No joystick found.")
    exit()

def transmit(id: int, p1: float, p2: float):
    payload = struct.pack('<iff', id, p1, p2)
    ser.write(payload)
    print(f"Tx ID: {id}, p1: {p1:.2f}, p2: {p2:.2f}")
    time.sleep(0.05)

joystick = pygame.joystick.Joystick(0)
joystick.init()
print("Joystick:", joystick.get_name())

clock = pygame.time.Clock()

try:
    while True:
        pygame.event.get()

        # Joystick input
        roll = joystick.get_axis(0)
        pitch = joystick.get_axis(1)
        if abs(roll - last_roll) > 0.1 or abs(pitch - last_pitch) > 0.1:
            transmit(0, roll, pitch)
            last_roll = roll
            last_pitch = pitch
        
        # Throttle
        throttle = (-joystick.get_axis(2)+1)/2
        if abs(throttle - last_throttle) > 0.1:
            transmit(3, throttle, 0)
            last_throttle = throttle
        
        # Button Inputs
        b2 = joystick.get_button(2)
        b3 = joystick.get_button(3)
        b14 = joystick.get_button(14)
        b12 = joystick.get_button(12)
        b13 = joystick.get_button(13)

        # Delta flap
        if b2 and not pb2:
            transmit(2, 1, 0)
        if b3 and not pb3:
            transmit(2, -1, 0)
            
        # Test surfaces
        if b14 and not pb14:
            transmit(4, 0, 0)
        
        pb2 = b2
        pb3 = b3
        pb14 = b14
        pb12 = b12
        pb13 = b13

except Exception as err:
    print(f"Exiting program: {err}")
