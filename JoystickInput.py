import struct
import serial
import time
import pygame

ser = serial.Serial("/dev/ttyACM0", 115200, timeout=1)

pygame.init()
pygame.joystick.init()

# Maybe make this not as janky :sob:
pb2 = False
pb3 = False
pREPLACE1 = False
pREPLACE2 = False
pb14 = False
pb12 = False
pb13 = False

buttons = [
    # int button number, bool current state, transmit struct [int id, float p1, float p2] (use [] for no function)
    [2, False, [2, 1, 0]],  # Increment flap level
    [3, False, [2, 1, 0]],  # Decrement flap level
    
    [REPLACEME, False, [1,0,-1]],  # Increment aileron trim (1 servo deg)
    [REPLACEME, False, [1,0,1]],  # Decrement aileron trim (1 servo deg)
    [REPLACEME, False, [1,1,-1]],  # Increment elevator trim (1 servo deg)
    [REPLACEME, False, [1,1,1]],  # Decrement elevator trim (1 servo deg)
    
    [12, False, []],
    [13, False, []],
    
    [14, False, [4, 0, 0]],  # Test surfaces
]

pRoll = 0
pPitch = 0
pThrottle = 0

if pygame.joystick.get_count() == 0:
    print("ERROR: No joystick found.")
    exit()


def transmit(id: int, p1: float, p2: float):
    payload = struct.pack("<iff", id, p1, p2)
    ser.write(payload)
    print(f"Tx ID: {id}, p1: {p1:.2f}, p2: {p2:.2f}")
    time.sleep(0.05)


joystick = pygame.joystick.Joystick(0)
joystick.init()
print("Joystick:", joystick.get_name())

try:
    while True:
        pygame.event.get()

        # Joystick input
        roll = joystick.get_axis(0)
        pitch = joystick.get_axis(1)
        if abs(roll - pRoll) > 0.07 or abs(pitch - pPitch) > 0.07:
            transmit(0, roll, pitch)

            pRoll = roll
            pPitch = pitch

        # Throttle
        throttle = (-joystick.get_axis(2) + 1) / 2
        if abs(throttle - pThrottle) > 0.1:
            transmit(3, throttle, 0)
            pThrottle = throttle

        # Button Inputs
        for btn in buttons:  # !! Verify !!
            state = joystick.get_button(btn[0])
            if state and not btn[1]:
                transmit(btn[2][0], btn[2][1], btn[2][2])
            btn[1] = state

except Exception as err:
    print(f"Exiting program: {err}")
