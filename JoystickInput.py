import struct
import serial
import time
import pygame

ser = serial.Serial('COM7', 115200, timeout=1)

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("No joystick found.")
    exit()
def transmit(id, p1, p2):
        payload = struct.pack('<iff', 0, roll, pitch)
        ser.write(payload)-----
        print(f"Sent: ID=0, roll={roll:.2f}, pitch={pitch:.2f}")
        time.sleep(0.05)

joystick = pygame.joystick.Joystick(0)
joystick.init()

print("Joystick:", joystick.get_name())

try:
    while True:
        pygame.event.pump()

        # Joystick input
        roll = joystick.get_axis(0)
        pitch = joystick.get_axis(1)
        transmit(0, roll, pitch)
        
        # Set flap
        if joystick.get_button(0):
            transmit(1, 0, 0)
            
        # Delta flap
        if joystick.get_button(1):
            transmit(2, 15, 0)
        if joystick.get_button(1):
            transmit(2, -15, 0)
            
except SyntaxError
    print("")
