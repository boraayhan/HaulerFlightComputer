import pygame
import time

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("ERROR: No joystick found.")
    exit()
js = pygame.joystick.Joystick(0)
js.init()

while True:
    pygame.event.pump()

    for i in range(js.get_numbuttons()):
        if js.get_button(i):
            print(f"[BUTTON] ID={i} pressed")
    for i in range(js.get_numaxes()):
        val = js.get_axis(i)
        if abs(val) > 0.1:   # deadzone
            print(f"[AXIS] ID={i} value={val:.3f}")
    for i in range(js.get_numhats()):
        hat = js.get_hat(i)
        if hat != (0, 0):
            print(f"[HAT] ID={i} value={hat}")
    time.sleep(0.05)