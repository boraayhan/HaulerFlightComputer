// LIBRARIES
#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <stdint.h>
#include "RF24.h"

#define RADIO_PIN_CE 7
#define RADIO_PIN_CSN 8

#define PROPELLER_PIN 49

// CONSTANTS
const float FLAPERON_RATIO_CONSTANT = 0; // 0 for flaperon mode off, 0.3 recommended

const float AILERON_POS_MIN = -80;
const float AILERON_POS_MAX = 80;

const float FLAP_POS_MAX = 0;
const float FLAP_POS_MIN = -40;

const float ELEVATOR_POS_MIN = -60;
const float ELEVATOR_POS_MAX = 60;

float flap = 0;

uint8_t address[][6] = {"1Node", "2Node"};

enum ControlSurfaces
{
  AILERON_LEFT = 0,
  AILERON_RIGHT,
  ELEVATOR_LEFT,
  ELEVATOR_RIGHT,
  num_surface
};

enum AutopilotDirections
{
  x = 0,
  y,
  z,
  num_direction
};

struct Payload
{
  int32_t id;
  float p1;
  float p2;
};

struct ControlSurface
{
  Servo servo;
  int pin;
  float zero;
  float min;
  float max;
  int dir; // +1 or -1 to correct for rotational symmetry
  void init()
  {
    servo.attach(pin);
    move(0);
    delay(1000);
  }

  void test()
  {
    for (float i = 0; i < max; i++)
    {
      move(i);
      delay(5);
    }

    for (float i = max; i > min; i--)
    {
      move(i);
      delay(5);
    }

    for (float i = min; i <= 0; i += 1.0)
    {
      move(i);
      delay(5);
    }
    move(0);
  }

  void move(float angle)
  {                                                                                     // Moves to specified angle, accounting for zero-level
    float target = constrain((angle + flap * FLAPERON_RATIO_CONSTANT), min, max) * dir; // TODO: Verify
    // Serial.println(target);
    servo.write(zero + target);
  }

  void trim(float angle)
  {
    zero += dir * angle;
  }
};

// VARIABLES AND OBJECTS
RF24 radio(RADIO_PIN_CE, RADIO_PIN_CSN);
Payload payload;
Servo propeller; // This is NOT a servo lmao

bool autopilot[3] = {false, false, false};

ControlSurface surfaces[num_surface] = {
    //{ Servo(), 2, 90, AILERON_POS_MIN, AILERON_POS_MAX, -1 },  // AILERON_LEFT
    {Servo(), 3, 90, AILERON_POS_MIN, AILERON_POS_MAX, -1}, // AILERON_RIGHT
                                                            // { Servo(), 4, 90, ELEVATOR_POS_MIN, ELEVATOR_POS_MAX, -1 },  // ELEVATOR_LEFT
                                                            // { Servo(), 5, 90, ELEVATOR_POS_MIN, ELEVATOR_POS_MAX, -1 },  // ELEVATOR_RIGHT
};

// FUNCTIONS
void setup()
{
  Serial.begin(115200);
  InitializeSystems();
  Serial.println("Initialized!");
}

void loop()
{
  ReceiveRadio();
}
void InitializeSystems()
{
  SetupRadio();
  // propeller.attach(PROPELLER_PIN);
  for (ControlSurface &s : surfaces)
  {
    s.init();
  }
}

void SetupRadio()
{ // Initializes radio
  if (!radio.begin())
  {
    Serial.println(F("Error: Radio hardware failure!"));
    while (1)
    {
    }
  }
  radio.setPALevel(RF24_PA_LOW); // FIXME: Experiment with different power levels. External power might be necessary
  radio.setPayloadSize(sizeof(Payload));
  radio.openReadingPipe(1, address[0]);
  radio.startListening();
  delay(1000); // Allow for radio to stabilize
}

void ReceiveRadio()
{ // Receives radio payload {id, p1, p2}, processes accordingly
  uint8_t pipe;
  if (radio.available(&pipe))
  {
    radio.read(&payload, sizeof(Payload)); // Receive the payload
    float p1 = payload.p1;
    float p2 = payload.p2;

    // For some reason, switch() case: didn't work for id's higher than 3? I spent 20 mins trying to bugfix it to no avail. I am so confused, but this works (I guess). I'm starting to suspect that it's emi caused by solar flares or something funky like that
    if (payload.id == 0)
    { // Joystick input
      MoveSurfacesWithJoystick(payload.p1, payload.p2);
      if (payload.p1 > 0.5) // Disengage rolling autopilot
        autopilot[x] = false;
      if (payload.p2 > 0.5) // Disengage pitching autopilot
        autopilot[y] = false;
    }

    if (payload.id == 3)
    {                                 // Throttle
      float speed = payload.p1 * 170; // FIXME: Fails often. Make it such that throttle never stalls at max
      SetThrottle(speed);
      // Serial.println(speed);
    }

    if (payload.id == 4)
    { // test surfaces
      TestSurfaces();
    }
  }
}

void MoveSurfacesWithJoystick(float jX, float jY)
{ // Translates payload data to aileron and elevator motion
  float pAileron = jX * (AILERON_POS_MAX - AILERON_POS_MIN);
  float pElevator = jY * (ELEVATOR_POS_MAX - ELEVATOR_POS_MIN);

  surfaces[AILERON_LEFT].move(pAileron);
  surfaces[AILERON_RIGHT].move(pAileron);
  surfaces[ELEVATOR_LEFT].move(pElevator);
  surfaces[ELEVATOR_RIGHT].move(pElevator);
}

void SetThrottle(float speed)
{
  propeller.write(speed);
}

void TestSurfaces()
{
  for (ControlSurface &s : surfaces)
  {
    s.test();
    Serial.println("Testing surface");
    delay(500);
  }
}