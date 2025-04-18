import pygame

pygame.init()
pygame.joystick.init()

jX = 0
jY = 0

if pygame.joystick.get_count() == 0:
    print("No joystick detected.")
    exit()

joystick = pygame.joystick.Joystick(0)
joystick.init()

print(f"Joystick detected: {joystick.get_name()}")
try:
    while True:
        pygame.event.pump()

        # Axes
        for i in range(joystick.get_numaxes()):
            axis = joystick.get_axis(i)
            print(f"Axis {i}: {axis:.3f}")

        # Buttons
        for i in range(joystick.get_numbuttons()):
            button = joystick.get_button(i)
            print(f"Button {i}: {'Pressed' if button else 'Released'}")

        # Hats (D-pad)
        for i in range(joystick.get_numhats()):
            hat = joystick.get_hat(i)
            print(f"Hat {i}: {hat}")

        print("----------")
        pygame.time.wait(200)

except KeyboardInterrupt:
    print("Exiting...")
