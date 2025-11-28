// LIBRARIES
#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <stdint.h>
#include <RF24.h>
#include <VL53L0X.h>

#define RADIO_PIN_CE 7
#define RADIO_PIN_CSN 8

#define PROPELLER_PIN 3

// CONSTANTS
const float FLAPERON_RATIO_CONSTANT = 0.0; // 0 for flaperon mode off, 0.3 recommended

const float AILERON_POS_MIN = -90;
const float AILERON_POS_MAX = 90;

const float ELEVATOR_POS_MIN = -90;
const float ELEVATOR_POS_MAX = 90;

const float AUTOPILOT_DISENGAGE_THRESHOLD = 0.3; // 0 to 1, how "hard" input is needed to disengage AP. > 0.5 is dangerous.
/*
const float KP_ROLL = 1; // FIXME: Tune!!!
const float KI_ROLL = 1; // FIXME: Tune!!!
const float KD_ROLL = 1; // FIXME: Tune!!!
*/

float flap = 0;
float throttle = 0;

uint8_t address[][6] = {"1Node", "2Node"};

enum ControlSurfaces
{
  AILERON_LEFT = 0,
  AILERON_RIGHT,
  ELEVATOR_LEFT,
  ELEVATOR_RIGHT,
  num_surface
};

/*enum HeightSensors
{
  LEFT = 0,
  RIGHT,
  // Can add more sensors for greater redundancy
  num_heightSensor
};*/

enum AutopilotDirections
{
  roll = 0,
  pitch,
  yaw,
};

struct Payload
{
  int32_t id;
  float p1;
  float p2;
};

/*struct HeightSensor
{
  VL53L0X sensor;
  int PIN_IRQ;
  int side; // -1 for left, +1 for right of fuselage relative to dir. of motion

  void init()
  {
    Serial.println("Make sure to implement L0X sensor");
  }

  float read()
  {
    return 0;
  }
}; */

struct ControlSurface
{
  Servo servo;
  int PIN_SIGNAL;
  float zero;
  float min;
  float max;
  int dir; // +1 or -1 to correct for rotational symmetry
  void init()
  {
    servo.attach(PIN_SIGNAL);
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
    {Servo(), 2, 90, AILERON_POS_MIN, AILERON_POS_MAX, -1},    // AILERON_LEFT
    {Servo(), 3, 90, AILERON_POS_MIN, AILERON_POS_MAX, -1},    // AILERON_RIGHT
    {Servo(), 4, 105, ELEVATOR_POS_MIN, ELEVATOR_POS_MAX, -1}, // ELEVATOR_LEFT
    {Servo(), 5, 100, ELEVATOR_POS_MIN, ELEVATOR_POS_MAX, 1},  // ELEVATOR_RIGHT
};

/*HeightSensor heights[num_heightSensor] =
};*/

// FUNCTIONS
void setup()
{
  Serial.begin(115200);
  InitializeSystems();
  Serial.println("Initialized!");
  delay(1000);
}

void loop()
{
  ReceiveRadio();
  SetThrottle();
  /*if (autopilot[roll])
  { // Maintains zero bank angle
    RollAutopilot();
  }
  if (autopilot[pitch])
  {
  }*/
}

void InitializeSystems()
{
  propeller.attach(PROPELLER_PIN);
  propeller.writeMicroseconds(1000);
  SetupRadio();
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
  radio.setPALevel(RF24_PA_LOW);
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
    int id = payload.id;
    float p1 = payload.p1;
    float p2 = payload.p2;

    if (id == 0)
    { // Joystick input
      MoveSurfaces(p1, p2);
      if (abs(p1) > AUTOPILOT_DISENGAGE_THRESHOLD || abs(p2) > AUTOPILOT_DISENGAGE_THRESHOLD)
      { // Disengage autopilot
        DisengageAutopilot();
      }
    }

    if (id == 1)
    {              // Trim
      if (p1 == 0) // Aileron trim
      {
        surfaces[AILERON_LEFT].trim(p2);
        surfaces[AILERON_RIGHT].trim(p2);
      }
      if (p1 == 1) // Elevator trim
      {
        surfaces[ELEVATOR_LEFT].trim(p2);
        surfaces[ELEVATOR_RIGHT].trim(p2);
      }
    }

    if (id == 3)
    { // Throttle
      if (p1 < 0.2)
      {
        throttle = 0;
      }
      else
      {
        throttle = p1;
      }
    }

    if (id == 4)
    { // test surfaces
      TestSurfaces();
    }
  }
}

void MoveSurfaces(float jR, float jP)
{                                                                // Translates a value (-1.00 to 1.00) to aileron and elevator motion
  float pAileron = jR * (AILERON_POS_MAX - AILERON_POS_MIN) / 2; // TODO: Verify whether div by 2 is correct
  float pElevator = jP * (ELEVATOR_POS_MAX - ELEVATOR_POS_MIN) / 2;

  surfaces[AILERON_LEFT].move(pAileron);
  surfaces[AILERON_RIGHT].move(pAileron);
  surfaces[ELEVATOR_LEFT].move(pElevator);
  surfaces[ELEVATOR_RIGHT].move(pElevator);
}

void SetThrottle()
{
  float speed = constrain(1000 + 1000 * throttle, 1000, 2000);
  propeller.writeMicroseconds(speed);
  Serial.println(speed);
}

void TestSurfaces()
{
  for (ControlSurface &s : surfaces)
  {
    s.test();
    Serial.println(F("Testing surface"));
    delay(500);
  }
  Serial.println(F("Testing complete!"));
}

/*void RollAutopilot()
{
  bool sensorsReady = true;
  if (sensorsReady)
  {
    float rollError = 0;
    for (HeightSensor &hs : heights)
    {
      rollError += hs.read() * hs.side; // Maybe normalize the net angle using trig, albeit to more computational cost
    }
    MoveSurfaces(KP_ROLL * constrain(rollError, -2.5, 2.5) / 2.5, 0); // Evil code, should work
  }
}
*/

void DisengageAutopilot()
{
  autopilot[roll] = false;
  autopilot[pitch] = false;
  autopilot[yaw] = false;
}